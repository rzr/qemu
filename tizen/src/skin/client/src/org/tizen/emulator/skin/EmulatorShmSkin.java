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
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.screenshot.ShmScreenShotWindow;
import org.tizen.emulator.skin.util.SkinUtil;

public class EmulatorShmSkin extends EmulatorSkin {
	private Logger logger = SkinLogger.getSkinLogger(
			EmulatorShmSkin.class).getLogger();

	public static final int RED_MASK = 0x00FF0000;
	public static final int GREEN_MASK = 0x0000FF00;
	public static final int BLUE_MASK = 0x000000FF;
	public static final int COLOR_DEPTH = 24; /* no need to Alpha channel */

	/* define JNI functions */
	public native int shmget(int shmkey, int size);
	public native int shmdt();
	public native int getPixels(int[] array);

	private PaletteData paletteData;
	private PollFBThread pollThread;

	class PollFBThread extends Thread {
		private Display display;
		private int widthFB;
		private int heightFB;
		private int[] arrayFramebuffer;
		private ImageData imageData;
		private Image framebuffer;

		private volatile boolean stopRequest;
		private Runnable runnable;
		private int intervalWait;

		public PollFBThread(int widthFB, int heightFB) {
			this.display = Display.getDefault();
			this.widthFB = widthFB;
			this.heightFB = heightFB;
			this.arrayFramebuffer = new int[widthFB * heightFB];
			this.imageData = new ImageData(widthFB, heightFB, COLOR_DEPTH, paletteData);
			this.framebuffer = new Image(Display.getDefault(), imageData);

			setDaemon(true);
			setWaitIntervalTime(30);

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

			int sizeFramebuffer = widthFB * heightFB;

			while (!stopRequest) {
				synchronized(this) {
					try {
						this.wait(intervalWait); /* ms */
					} catch (InterruptedException e) {
						e.printStackTrace();
						break;
					}
				}

				int result = getPixels(arrayFramebuffer); /* from shared memory */
				//logger.info("getPixels native function returned " + result);

				imageData.setPixels(0, 0, sizeFramebuffer, arrayFramebuffer, 0);

				display.syncExec(new Runnable() {
					@Override
					public void run() {
						framebuffer.dispose();
						framebuffer = new Image(display, imageData);
					}
				});

				if (display.isDisposed() == false) {
					/* redraw canvas */
					display.asyncExec(runnable);
				}
			}

			logger.info("PollFBThread is stopped");

			int result = shmdt();
			logger.info("shmdt native function returned " + result);
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
	public EmulatorShmSkin(EmulatorSkinState state, EmulatorFingers finger,
			EmulatorConfig config, SkinInformation skinInfo, boolean isOnTop) {
		super(state, finger, config, skinInfo, SWT.NONE, isOnTop);
		this.paletteData = new PaletteData(RED_MASK, GREEN_MASK, BLUE_MASK);
	}

	protected void skinFinalize() {
		pollThread.stopRequest();

		super.finger.setMultiTouchEnable(0);
		super.finger.clearFingerSlot();
		super.finger.cleanup_multiTouchState();

		super.skinFinalize();
	}

	public long initLayout() {
		super.initLayout();

		/* base + 1 = sdb port */
		/* base + 2 = shared memory key */
		int shmkey = config.getArgInt(ArgsConstants.NET_BASE_PORT) + 2;
		logger.info("shmkey = " + shmkey);

		/* initialize shared memory */
		int result = shmget(shmkey,
				currentState.getCurrentResolutionWidth() *
				currentState.getCurrentResolutionHeight() * 4);
		logger.info("shmget native function returned " + result);

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

				if (pollThread.getWaitIntervalTime() == 0) {
					logger.info("draw black screen");

					e.gc.drawRectangle(-1, -1, x + 1, y + 1);
					return;
				}

				if (currentState.getCurrentAngle() == 0) { /* portrait */
					e.gc.drawImage(pollThread.framebuffer,
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
				e.gc.drawImage(pollThread.framebuffer,
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
	public void displayOn() {
		logger.info("display on");

		if (pollThread.isAlive()) {
			pollThread.setWaitIntervalTime(30);

			synchronized(pollThread) {
				pollThread.notify();
			}
		}
	}

	@Override
	public void displayOff() {
		logger.info("display off");

		if (isDisplayDragging == true) {
			logger.info("auto release : mouseEvent");
			MouseEventData mouseEventData = new MouseEventData(
					0, MouseEventType.RELEASE.value(),
					0, 0, 0, 0, 0);

			communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData);
		}

		if (pollThread.isAlive()) {
			pollThread.setWaitIntervalTime(0);

			shell.getDisplay().asyncExec(new Runnable() {
				@Override
				public void run() {
					lcdCanvas.redraw();
				}
			});
		}
	}

	@Override
	protected void openScreenShotWindow() {
		if (screenShotDialog != null) {
			logger.info("screenshot window was already opened");
			return;
		}

		try {
			screenShotDialog = new ShmScreenShotWindow(shell, communicator, this, config,
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
