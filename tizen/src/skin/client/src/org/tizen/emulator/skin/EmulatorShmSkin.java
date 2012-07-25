package org.tizen.emulator.skin;

import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.config.EmulatorConfig;

public class EmulatorShmSkin extends EmulatorSkin {
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
					lcdCanvas.redraw();
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

				display.asyncExec(runnable); //redraw canvas
			}
		}

		public void stopRequest() {
			stopRequest = true;
		}
	}

	/**
	 *  Constructor
	 */
	public EmulatorShmSkin(EmulatorConfig config, boolean isOnTop) {
		super(config, isOnTop);

		this.paletteData = new PaletteData(RED_MASK, GREEN_MASK, BLUE_MASK);
	}

	public long compose() {
		long ret = super.compose();

		int result = shmget(currentLcdWidth * currentLcdHeight); //initialize shared memory
		//logger.info("shmget navtive function returned " + result);

		pollThread = new PollFBThread(currentLcdWidth, currentLcdHeight); //update lcd thread

		lcdCanvas.addPaintListener(new PaintListener() {
			public void paintControl(PaintEvent e) {
				/* e.gc.setAdvanced(true);
				if (!e.gc.getAdvanced()) {
					logger.info("Advanced graphics not supported");
				} */

				int x = lcdCanvas.getSize().x;
				int y = lcdCanvas.getSize().y;
				if (currentAngle == 0) { //portrait
					e.gc.drawImage(pollThread.framebuffer,
							0, 0, pollThread.lcdWidth, pollThread.lcdHeight,
							0, 0, x, y);
					return;
				}

				Transform transform = new Transform(lcdCanvas.getDisplay());
				transform.rotate(currentAngle);

				if (currentAngle == 90) { //reverse landscape
					int temp;
					temp = x;
					x = y;
					y = temp;
					transform.translate(0, y * -1);
				} else if (currentAngle == 180) { //reverse portrait
					transform.translate(x * -1, y * -1);
				} else if (currentAngle == -90) { //landscape
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

		return ret;
	}
}
