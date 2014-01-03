/**
 * Emulator Skin Process
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

package org.tizen.emulator.skin;

import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseButtonType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.comm.sock.data.StartData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.dbi.OptionType;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ShmScreenShotWindow;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class EmulatorShmSkin extends EmulatorSkin {
	public static final int DISPLAY_COLOR_DEPTH = 24; /* no need to Alpha channel */

	/* touch values */
	protected static int pressingX = -1, pressingY = -1;
	protected static int pressingOriginX = -1, pressingOriginY = -1;

	private static Logger logger = SkinLogger.getSkinLogger(
			EmulatorShmSkin.class).getLogger();

	static {
		/* load JNI library file */
		try {
			System.loadLibrary("shared");
		} catch (UnsatisfiedLinkError e) {
			logger.info("Failed to load a JNI library file.\n" + e);

			Shell temp = new Shell(Display.getDefault());
			MessageBox messageBox = new MessageBox(temp, SWT.ICON_ERROR);
			messageBox.setText("Emulator");
			messageBox.setMessage(
					"Failed to load a JNI library file.\n\n" + e);
			messageBox.open();
			temp.dispose();

			EmulatorSkinMain.terminateImmediately(-1);;
		}
	}

	/* define JNI functions */
	public native int shmget(int shmkey, int size);
	public native int shmdt();
	public native int getPixels(int[] array);

	private PaletteData palette;
	private PollFBThread pollThread;
	private Image imageGuide;

	private int maxTouchPoint;
	private EmulatorFingers finger;
	private int multiTouchKey;
	private int multiTouchKeySub;

	class PollFBThread extends Thread {
		private Display display;
		private int widthFB;
		private int heightFB;
		private int sizeFramebuffer;
		private int[] arrayFramebuffer;
		private ImageData dataFramebuffer;
		private Image imageFramebuffer;
		private Image imageTemp;

		private volatile boolean stopRequest;
		private Runnable runnable;
		private int intervalWait;

		/**
		 *  Constructor
		 */
		public PollFBThread(PaletteData paletteDisplay, int widthFB, int heightFB) {
			this.display = Display.getDefault();
			this.widthFB = widthFB;
			this.heightFB = heightFB;
			this.sizeFramebuffer = widthFB * heightFB;
			this.arrayFramebuffer = new int[sizeFramebuffer];

			this.dataFramebuffer =
					new ImageData(widthFB, heightFB, DISPLAY_COLOR_DEPTH, paletteDisplay);
			this.imageFramebuffer =
					new Image(Display.getDefault(), dataFramebuffer);

			setName("PollFBThread");
			setDaemon(true);
			setWaitIntervalTime(0);

			this.runnable = new Runnable() {
				@Override
				public void run() {
					// logger.info("update display framebuffer");
					if (lcdCanvas.isDisposed() == false) {
						lcdCanvas.redraw();
					}
				}
			};
		}

		public synchronized void setWaitIntervalTime(int ms) {
			intervalWait = ms;
		}

		public synchronized int getWaitIntervalTime() {
			return intervalWait;
		}

		@Override
		public void run() {
			stopRequest = false;

			while (!stopRequest) {
				synchronized(this) {
					try {
						this.wait(intervalWait); /* ms */
					} catch (InterruptedException e) {
						e.printStackTrace();
						break;
					}
				}

				if (stopRequest == true) {
					break;
				}

				if (display.isDisposed() == false) {
					/* redraw canvas */
					display.asyncExec(runnable);
				}
			}

			logger.info("PollFBThread is stopped");

			int result = shmdt();
			logger.info("shmdt native function returned " + result);
		}

		public void getPixelsFromSharedMemory() {
			//int result =
			getPixels(arrayFramebuffer);
			//logger.info("getPixels native function returned " + result);

			communicator.sendToQEMU(
					SendCommand.RESPONSE_DRAW_FRAME, null, true);

			dataFramebuffer.setPixels(0, 0,
					sizeFramebuffer, arrayFramebuffer, 0);

			imageTemp = imageFramebuffer;
			imageFramebuffer = new Image(display, dataFramebuffer);
			imageTemp.dispose();
		}

		public void stopRequest() {
			stopRequest = true;

			synchronized(pollThread) {
				pollThread.notify();
			}
		}
	}

	/**
	 *  Constructor
	 */
	public EmulatorShmSkin(EmulatorConfig config,
			SkinInformation skinInfo, boolean isOnTop) {
		super(config, skinInfo, SWT.NONE, isOnTop);

		/* ARGB */
		this.palette = new PaletteData(0x00FF0000, 0x0000FF00, 0x000000FF);

		/* get MaxTouchPoint from startup argument */
		this.maxTouchPoint = config.getArgInt(
				ArgsConstants.INPUT_TOUCH_MAXPOINT);
	}

	@Override
	protected void skinFinalize() {
		pollThread.stopRequest();

		/* remove multi-touch finger points */
		finger.cleanupMultiTouchState();

		super.skinFinalize();
	}

	@Override
	public StartData initSkin() {
		initLayout();

		finger = new EmulatorFingers(maxTouchPoint,
				currentState, communicator, palette);

		if (SwtUtil.isMacPlatform() == true) {
			multiTouchKey = SWT.COMMAND;
		} else {
			multiTouchKey = SWT.CTRL;
		}
		multiTouchKeySub = SWT.SHIFT;

		initDisplay();

		/* generate a start data */
		int width = getEmulatorSkinState().getCurrentResolutionWidth();
		int height = getEmulatorSkinState().getCurrentResolutionHeight();
		int scale = getEmulatorSkinState().getCurrentScale();
		short rotation = getEmulatorSkinState().getCurrentRotationId();

		boolean isBlankGuide = true;
		OptionType option = config.getDbiContents().getOption();
		if (option != null) {
			isBlankGuide = (option.getBlankGuide() == null) ?
					true : option.getBlankGuide().isVisible();
		}

		StartData startData = new StartData(0,
				width, height, scale, rotation, isBlankGuide);
		logger.info("" + startData);

		return startData;
	}

	private void initDisplay() {
		/* initialize shared memory */
		/* base + 1 = sdb port */
		/* base + 2 = shared memory key */
		int shmkey = config.getArgInt(ArgsConstants.VM_BASE_PORT) + 2;
		logger.info("shmkey = " + shmkey);

		int result = shmget(shmkey,
				currentState.getCurrentResolutionWidth() *
				currentState.getCurrentResolutionHeight() * 4);
		logger.info("shmget native function returned " + result);

		if (result == 1) {
			logger.severe("Failed to get identifier of the shared memory segment.");
			SkinUtil.openMessage(shell, null,
					"Cannot launch this VM.\n" +
					"Failed to get identifier of the shared memory segment.",
					SWT.ICON_ERROR, config);

			EmulatorSkinMain.terminateImmediately(-1);
		} else if (result == 2) {
			logger.severe("Failed to attach the shared memory segment.");
			SkinUtil.openMessage(shell, null,
					"Cannot launch this VM.\n" +
					"Failed to attach the shared memory segment.",
					SWT.ICON_ERROR, config);

			EmulatorSkinMain.terminateImmediately(-1);
		}

		/* display updater thread */
		pollThread = new PollFBThread(palette,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight());

		lcdCanvas.addPaintListener(new PaintListener() {
			public void paintControl(PaintEvent e) { //TODO: optimize
				/* e.gc.setAdvanced(true);
				if (!e.gc.getAdvanced()) {
					logger.info("Advanced graphics not supported");
				} */

				final int screen_width = lcdCanvas.getSize().x;
				final int screen_height = lcdCanvas.getSize().y;

				/* if (pollThread.getWaitIntervalTime() == 0) {
					logger.info("draw black screen");

					e.gc.drawRectangle(-1, -1, x + 1, y + 1);
					return;
				}*/

				if (isOnInterpolation == false) {
					/* Mac - NSImageInterpolationNone */
					e.gc.setInterpolation(SWT.NONE);
				} else {
					/* Mac - NSImageInterpolationHigh */
					e.gc.setInterpolation(SWT.HIGH);
				}

				if (currentState.getCurrentAngle() == 0) { /* portrait */
					e.gc.drawImage(pollThread.imageFramebuffer,
							0, 0, pollThread.widthFB, pollThread.heightFB,
							0, 0, screen_width, screen_height);
				} else { /* non-portrait */
					Transform newTransform = new Transform(lcdCanvas.getDisplay());
					Transform oldTransform = new Transform(lcdCanvas.getDisplay());
					newTransform.rotate(currentState.getCurrentAngle());

					int frame_width = 0, frame_height = 0;
					if (currentState.getCurrentAngle() == 90) { /* reverse landscape */
						frame_width = screen_height;
						frame_height = screen_width;
						newTransform.translate(0, frame_height * -1);
					} else if (currentState.getCurrentAngle() == 180) { /* reverse portrait */
						frame_width = screen_width;
						frame_height = screen_height;
						newTransform.translate(frame_width * -1, frame_height * -1);
					} else if (currentState.getCurrentAngle() == -90) { /* landscape */
						frame_width = screen_height;
						frame_height = screen_width;
						newTransform.translate(frame_width * -1, 0);
					}

					/* save current transform as oldTransform */
					e.gc.getTransform(oldTransform);
					/* set to new transform */
					e.gc.setTransform(newTransform);
					e.gc.drawImage(pollThread.imageFramebuffer,
							0, 0, pollThread.widthFB, pollThread.heightFB,
							0, 0, frame_width, frame_height);
					/* back to old transform */
					e.gc.setTransform(oldTransform);

					newTransform.dispose();
					oldTransform.dispose();
				}

				/* blank guide */
				if (imageGuide != null) {
					logger.info("draw blank guide");

					int widthImage = imageGuide.getImageData().width;
					int heightImage = imageGuide.getImageData().height;
					int margin_w = screen_width - widthImage;
					int margin_h = screen_height - heightImage;
					int margin = Math.min(margin_w, margin_h);

					int scaledWidthImage = widthImage;
					int scaledHeightImage = heightImage;
					if (margin < 0) {
						scaledWidthImage += margin;
						scaledHeightImage += margin;
					}

					e.gc.drawImage(imageGuide, 0, 0,
							widthImage, heightImage,
							(screen_width - scaledWidthImage) / 2,
							(screen_height - scaledHeightImage) / 2,
							scaledWidthImage, scaledHeightImage);

					imageGuide = null;
				} else {
					/* draw finger image for when rotate while use multitouch */
					if (finger.getMultiTouchEnable() == 1 ||
							finger.getMultiTouchEnable() == 2) {
						finger.rearrangeFingerPoints(currentState.getCurrentResolutionWidth(),
								currentState.getCurrentResolutionHeight(),
								currentState.getCurrentScale(),
								currentState.getCurrentRotationId());
					}

					finger.drawFingerPoints(e.gc);
				}
			}
		});

		pollThread.start();
	}

	@Override
	public void updateDisplay() {
		pollThread.getPixelsFromSharedMemory();

		synchronized(pollThread) {
			pollThread.notify();
		}
	}

	@Override
	public void drawImageToDisplay(Image image) {
		imageGuide = image;

		Display.getDefault().syncExec(new Runnable() {
			@Override
			public void run() {
				lcdCanvas.redraw();
			}
		});
	}

	@Override
	public void displayOn() {
		super.displayOn();
	}

	@Override
	public void displayOff() {
		super.displayOff();

		/*if (pollThread.isAlive()) {
			pollThread.setWaitIntervalTime(0);

			shell.getDisplay().asyncExec(new Runnable() {
				@Override
				public void run() {
					lcdCanvas.redraw();
				}
			});
		}*/
	}

	/* mouse event */
	@Override
	protected void mouseMoveDelivery(MouseEvent e, int eventType) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());

		//eventType = MouseEventType.DRAG.value();
		if (finger.getMultiTouchEnable() == 1) {
			finger.maruFingerProcessing1(eventType,
					e.x, e.y, geometry[0], geometry[1]);
			return;
		} else if (finger.getMultiTouchEnable() == 2) {
			finger.maruFingerProcessing2(eventType,
					e.x, e.y, geometry[0], geometry[1]);
			return;
		}

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), eventType,
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(
				SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
	}

	@Override
	protected void mouseUpDelivery(MouseEvent e) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());
		logger.info("mouseUp in display" +
				" x:" + geometry[0] + " y:" + geometry[1]);

		pressingX = pressingY = -1;
		pressingOriginX = pressingOriginY = -1;

		if (finger.getMultiTouchEnable() == 1) {
			logger.info("maruFingerProcessing 1");
			finger.maruFingerProcessing1(MouseEventType.RELEASE.value(),
					e.x, e.y, geometry[0], geometry[1]);
			return;
		} else if (finger.getMultiTouchEnable() == 2) {
			logger.info("maruFingerProcessing 2");
			finger.maruFingerProcessing2(MouseEventType.RELEASE.value(),
					e.x, e.y, geometry[0], geometry[1]);
			return;
		}

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(
				SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
	}

	@Override
	protected void mouseDownDelivery(MouseEvent e) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());
		logger.info("mouseDown in display" +
				" x:" + geometry[0] + " y:" + geometry[1]);

		pressingX = geometry[0];
		pressingY = geometry[1];
		pressingOriginX = e.x;
		pressingOriginY = e.y;

		if (finger.getMultiTouchEnable() == 1) {
			logger.info("maruFingerProcessing 1");
			finger.maruFingerProcessing1(MouseEventType.PRESS.value(),
					e.x, e.y, geometry[0], geometry[1]);
			return;
		} else if (finger.getMultiTouchEnable() == 2) {
			logger.info("maruFingerProcessing 2");
			finger.maruFingerProcessing2(MouseEventType.PRESS.value(),
					e.x, e.y, geometry[0], geometry[1]);
			return;
		}

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(
				SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
	}

	/* keyboard event */
	@Override
	protected void keyReleasedDelivery(int keyCode, int stateMask, int keyLocation) {
		/* check multi-touch */
		if (keyCode == multiTouchKeySub || keyCode == multiTouchKey) {
			int tempStateMask = stateMask & ~SWT.BUTTON1;

			if (tempStateMask == (multiTouchKeySub | multiTouchKey)) {
				finger.setMultiTouchEnable(1);

				logger.info("enable multi-touch = mode 1");
			} else {
				finger.clearFingerSlot(false);

				logger.info("disable multi-touch");
			}
		}

		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.RELEASED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEYBOARD_KEY_EVENT, keyEventData, false);

		removePressedKeyFromList(keyEventData);
	}

	@Override
	protected void keyPressedDelivery(int keyCode, int stateMask, int keyLocation) {
		/* TODO: (finger.getMaxTouchPoint() > 1) */

		int tempStateMask = stateMask & ~SWT.BUTTON1;

		if ((keyCode == multiTouchKeySub && (tempStateMask & multiTouchKey) != 0) ||
				(keyCode == multiTouchKey && (tempStateMask & multiTouchKeySub) != 0))
		{
			finger.setMultiTouchEnable(2);

			/* add a finger before start the multi-touch processing
			if already exist the pressed touch in display */
			if (pressingX != -1 && pressingY != -1 &&
					pressingOriginX != -1 && pressingOriginY != -1) {
				finger.addFingerPoint(
						pressingOriginX, pressingOriginY,
						pressingX, pressingY);
				pressingX = pressingY = -1;
				pressingOriginX = pressingOriginY = -1;
			}

			logger.info("enable multi-touch = mode 2");
		} else if (keyCode == multiTouchKeySub || keyCode == multiTouchKey) {
			finger.setMultiTouchEnable(1);

			/* add a finger before start the multi-touch processing
			if already exist the pressed touch in display */
			if (pressingX != -1 && pressingY != -1 &&
					pressingOriginX != -1 && pressingOriginY != -1) {
				finger.addFingerPoint(
						pressingOriginX, pressingOriginY,
						pressingX, pressingY);
				pressingX = pressingY = -1;
				pressingOriginX = pressingOriginY = -1;
			}

			logger.info("enable multi-touch = mode 1");
		}

		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.PRESSED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEYBOARD_KEY_EVENT, keyEventData, false);

		addPressedKeyToList(keyEventData);
	}

	@Override
	protected void openScreenShotWindow() {
		if (screenShotDialog != null) {
			logger.info("screenshot window was already opened");
			return;
		}

		screenShotDialog = new ShmScreenShotWindow(this, config,
				palette, imageRegistry.getIcon(IconName.SCREENSHOT));

		try {
			screenShotDialog.open();
		} catch (ScreenShotException ex) {
			screenShotDialog = null;
			logger.log(Level.SEVERE, ex.getMessage(), ex);

			SkinUtil.openMessage(shell, null,
					"Fail to create a screen shot.", SWT.ICON_ERROR, config);

		} catch (Exception ex) {
			screenShotDialog = null;
			logger.log(Level.SEVERE, ex.getMessage(), ex);

			SkinUtil.openMessage(shell, null, "ScreenShot is not ready.\n" +
					"Please wait until the emulator is completely boot up.",
					SWT.ICON_WARNING, config);
		}
	}
}
