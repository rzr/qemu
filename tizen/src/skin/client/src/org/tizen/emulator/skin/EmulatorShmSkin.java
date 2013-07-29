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
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ShmScreenShotWindow;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class EmulatorShmSkin extends EmulatorSkin {
	public static final int RED_MASK = 0x00FF0000;
	public static final int GREEN_MASK = 0x0000FF00;
	public static final int BLUE_MASK = 0x000000FF;
	public static final int COLOR_DEPTH = 24; /* no need to Alpha channel */

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

			System.exit(-1);
		}
	}

	/* define JNI functions */
	public native int shmget(int shmkey, int size);
	public native int shmdt();
	public native int getPixels(int[] array);

	private PaletteData paletteData;
	private PollFBThread pollThread;

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

		public PollFBThread(int widthFB, int heightFB) {
			this.display = Display.getDefault();
			this.widthFB = widthFB;
			this.heightFB = heightFB;
			this.sizeFramebuffer = widthFB * heightFB;
			this.arrayFramebuffer = new int[sizeFramebuffer];

			this.dataFramebuffer =
					new ImageData(widthFB, heightFB, COLOR_DEPTH, paletteData);
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
			int result = getPixels(arrayFramebuffer);
			//logger.info("getPixels native function returned " + result);

			communicator.sendToQEMU(
					SendCommand.RESPONSE_DRAW_FRAMEBUFFER, null, true);

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

		this.paletteData = new PaletteData(RED_MASK, GREEN_MASK, BLUE_MASK);

		/* get MaxTouchPoint from startup argument */
		this.maxTouchPoint = config.getArgInt(
				ArgsConstants.INPUT_TOUCH_MAXPOINT);
	}

	protected void skinFinalize() {
		pollThread.stopRequest();

		finger.setMultiTouchEnable(0);
		finger.clearFingerSlot();
		finger.cleanup_multiTouchState();

		super.skinFinalize();
	}

	public long initLayout() {
		super.initLayout();

		finger = new EmulatorFingers(maxTouchPoint, currentState, communicator);
		if (SwtUtil.isMacPlatform() == true) {
			multiTouchKey = SWT.COMMAND;
		} else {
			multiTouchKey = SWT.CTRL;
		}
		multiTouchKeySub = SWT.SHIFT;

		/* base + 1 = sdb port */
		/* base + 2 = shared memory key */
		int shmkey = config.getArgInt(ArgsConstants.NET_BASE_PORT) + 2;
		logger.info("shmkey = " + shmkey);

		/* initialize shared memory */
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
			System.exit(-1);
		} else if (result == 2) {
			logger.severe("Failed to attach the shared memory segment.");
			SkinUtil.openMessage(shell, null,
					"Cannot launch this VM.\n" +
					"Failed to attach the shared memory segment.",
					SWT.ICON_ERROR, config);
			System.exit(-1);
		}

		/* update lcd thread */
		pollThread = new PollFBThread(
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight());

		lcdCanvas.addPaintListener(new PaintListener() {
			public void paintControl(PaintEvent e) { //TODO: optimize
				/* e.gc.setAdvanced(true);
				if (!e.gc.getAdvanced()) {
					logger.info("Advanced graphics not supported");
				} */

				int x = lcdCanvas.getSize().x;
				int y = lcdCanvas.getSize().y;

				/* if (pollThread.getWaitIntervalTime() == 0) {
					logger.info("draw black screen");

					e.gc.drawRectangle(-1, -1, x + 1, y + 1);
					return;
				}*/

				if (currentState.getCurrentAngle() == 0) { /* portrait */
					e.gc.drawImage(pollThread.imageFramebuffer,
							0, 0, pollThread.widthFB, pollThread.heightFB,
							0, 0, x, y);

					if (finger.getMultiTouchEnable() == 1 ||
							finger.getMultiTouchEnable() == 2) {
						finger.rearrangeFingerPoints(currentState.getCurrentResolutionWidth(), 
								currentState.getCurrentResolutionHeight(), 
								currentState.getCurrentScale(), 
								currentState.getCurrentRotationId());
					}

					finger.drawImage(e, currentState.getCurrentAngle());
					return;
				}

				Transform transform = new Transform(lcdCanvas.getDisplay());
				Transform oldtransform = new Transform(lcdCanvas.getDisplay());
				transform.rotate(currentState.getCurrentAngle());

				if (currentState.getCurrentAngle() == 90) { /* reverse landscape */
					int temp;
					temp = x;
					x = y;
					y = temp;
					transform.translate(0, y * -1);
				} else if (currentState.getCurrentAngle() == 180) { /* reverse portrait */
					transform.translate(x * -1, y * -1);
				} else if (currentState.getCurrentAngle() == -90) { /* landscape */
					int temp;
					temp = x;
					x = y;
					y = temp;
					transform.translate(x * -1, 0);
				}

				/* draw finger image for when rotate while use multitouch */
				if (finger.getMultiTouchEnable() == 1) {
					finger.rearrangeFingerPoints(currentState.getCurrentResolutionWidth(), 
							currentState.getCurrentResolutionHeight(), 
							currentState.getCurrentScale(), 
							currentState.getCurrentRotationId());
				}
				/* save current transform as "oldtransform" */
				e.gc.getTransform(oldtransform);
				/* set to new transfrom */
				e.gc.setTransform(transform);
				e.gc.drawImage(pollThread.imageFramebuffer,
						0, 0, pollThread.widthFB, pollThread.heightFB,
						0, 0, x, y);
				/* back to old transform */
				e.gc.setTransform(oldtransform);

				transform.dispose();
				finger.drawImage(e, currentState.getCurrentAngle());
			}
		});

		pollThread.start();

		return 0;
	}

	@Override
	public void updateDisplay() {
		pollThread.getPixelsFromSharedMemory();

		synchronized(pollThread) {
			pollThread.notify();
		}
	}

	@Override
	public void displayOn() {
		logger.info("display on");

//		if (pollThread.isAlive()) {
//			pollThread.setWaitIntervalTime(30);
//
//			synchronized(pollThread) {
//				pollThread.notify();
//			}
//		}
	}

	@Override
	public void displayOff() {
		logger.info("display off");

//		if (pollThread.isAlive()) {
//			pollThread.setWaitIntervalTime(0);
//
//			shell.getDisplay().asyncExec(new Runnable() {
//				@Override
//				public void run() {
//					lcdCanvas.redraw();
//				}
//			});
//		}
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

				logger.info("enable multi-touch = mode1");
			} else {
				finger.setMultiTouchEnable(0);
				finger.clearFingerSlot();

				logger.info("disable multi-touch");
			}
		}

		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.RELEASED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEY_EVENT, keyEventData, false);

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

			logger.info("enable multi-touch = mode2");
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

			logger.info("enable multi-touch = mode1");
		}

		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.PRESSED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEY_EVENT, keyEventData, false);

		addPressedKeyToList(keyEventData);
	}

	@Override
	protected void openScreenShotWindow() {
		if (screenShotDialog != null) {
			logger.info("screenshot window was already opened");
			return;
		}

		try {
			screenShotDialog = new ShmScreenShotWindow(shell, this, config,
					imageRegistry.getIcon(IconName.SCREENSHOT));
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
