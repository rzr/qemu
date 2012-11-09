/**
 * Emulator Skin Process
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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
import org.tizen.emulator.skin.config.EmulatorConfig;
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
	public static final int COLOR_DEPTH = 32;

	/* define JNI functions */
	public native int shmget(int size);
	public native int shmdt();
	public native int getPixels(int[] array);

	PaletteData paletteData;
	public PollFBThread pollThread;

	class PollFBThread extends Thread {
		private Display display;
		private int lcdWidth;
		private int lcdHeight;
		private int[] array;
		private ImageData imageData;
		private Image framebuffer;

		private volatile boolean stopRequest;
		private Runnable runnable;

		public PollFBThread(int lcdWidth, int lcdHeight) {
			this.display = Display.getDefault();
			this.lcdWidth = lcdWidth;
			this.lcdHeight = lcdHeight;
			this.array = new int[lcdWidth * lcdHeight];
			this.imageData = new ImageData(lcdWidth, lcdHeight, COLOR_DEPTH, paletteData);
			this.framebuffer = new Image(Display.getDefault(), imageData);

			this.runnable = new Runnable() {
				public void run() {
					// logger.info("update lcd framebuffer");
					if(lcdCanvas.isDisposed() == false) {
						lcdCanvas.redraw();
					}
				}
			};
		}

		public void run() {
			stopRequest = false;

			while (!stopRequest) {
				synchronized (this) {
					try {
						this.wait(30); //30ms
					} catch (InterruptedException ex) {
						break;
					}
				}

				int result = getPixels(array); //from shared memory
				//logger.info("getPixels navtive function returned " + result);

				for (int i = 0; i < lcdHeight; i++) {
					imageData.setPixels(0, i, lcdWidth, array, i * lcdWidth);
				}

				Image temp = framebuffer;
				framebuffer = new Image(display, imageData);
				temp.dispose();
				if(display.isDisposed() == false) {
					display.asyncExec(runnable); //redraw canvas
				}
			}

			//logger.info("PollFBThread is stopped");
		}

		public void stopRequest() {
			stopRequest = true;
		}
	}

	/**
	 *  Constructor
	 */
	public EmulatorShmSkin(EmulatorConfig config, SkinInformation skinInfo, boolean isOnTop) {
		super(config, skinInfo, isOnTop);

		this.paletteData = new PaletteData(RED_MASK, GREEN_MASK, BLUE_MASK);
	}

	protected void skinFinalize() {
		pollThread.stopRequest();
	}

	public long compose() {
		super.compose();

		/* initialize shared memory */
		int result = shmget(
				currentState.getCurrentResolutionWidth() *
				currentState.getCurrentResolutionHeight());
		//logger.info("shmget navtive function returned " + result);

		/* update lcd thread */
		pollThread = new PollFBThread(
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight());

		lcdCanvas.addPaintListener(new PaintListener() {
			public void paintControl(PaintEvent e) {
				/* e.gc.setAdvanced(true);
				if (!e.gc.getAdvanced()) {
					logger.info("Advanced graphics not supported");
				} */

				int x = lcdCanvas.getSize().x;
				int y = lcdCanvas.getSize().y;
				if (currentState.getCurrentAngle() == 0) { /* portrait */
					e.gc.drawImage(pollThread.framebuffer,
							0, 0, pollThread.lcdWidth, pollThread.lcdHeight,
							0, 0, x, y);
					return;
				}

				Transform transform = new Transform(lcdCanvas.getDisplay());
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

				e.gc.setTransform(transform);
				e.gc.drawImage(pollThread.framebuffer,
						0, 0, pollThread.lcdWidth, pollThread.lcdHeight,
						0, 0, x, y);

				transform.dispose();
			}
		});

		pollThread.start();

		return 0;
	}

	protected void openScreenShotWindow() {
		if (screenShotDialog != null) {
			return;
		}

		try {
			screenShotDialog = new ShmScreenShotWindow(shell, communicator, this, config,
					imageRegistry.getIcon(IconName.SCREENSHOT));
			screenShotDialog.open();

		} catch (ScreenShotException ex) {
			logger.log(Level.SEVERE, ex.getMessage(), ex);
			SkinUtil.openMessage(shell, null,
					"Fail to create a screen shot.", SWT.ICON_ERROR, config);

		} catch (Exception ex) {
			// defense exception handling.
			logger.log(Level.SEVERE, ex.getMessage(), ex);
			String errorMessage = "Internal Error.\n[" + ex.getMessage() + "]";
			SkinUtil.openMessage(shell, null, errorMessage, SWT.ICON_ERROR, config);

		} finally {
			screenShotDialog = null;
		}
	}
}
