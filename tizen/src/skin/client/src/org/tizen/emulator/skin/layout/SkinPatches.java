/**
 *
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

package org.tizen.emulator.skin.layout;

import java.util.logging.Logger;

import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.log.SkinLogger;

public class SkinPatches {
	private Logger logger =
			SkinLogger.getSkinLogger(SkinPatches.class).getLogger();

	private String pathImage;
	private int patchWidth;
	private int patchHeight;

	/* nine patches */
	private Image imageLT; /* left-top */
	private Image imageCT; /* center-top */
	private Image imageRT; /* right-top */

	private Image imageLC;
	private Image imageRC;

	private Image imageLB; /* left-bottom */
	private Image imageCB;
	private Image imageRB;

	// TODO: configurable
	private static final String filenameLT = "LT.png";
	private static final String filenameCT = "CT.png";
	private static final String filenameRT = "RT.png";

	private static final String filenameLC = "LC.png";
	private static final String filenameRC = "RC.png";

	private static final String filenameLB = "LB.png";
	private static final String filenameCB = "CB.png";
	private static final String filenameRB = "RB.png";

	public SkinPatches(String path) {
		this.pathImage = path;

		loadPatches(pathImage);

		// TODO:
		this.patchWidth = imageLT.getImageData().width;
		this.patchHeight = imageLT.getImageData().height;
		logger.info("patch size : " + patchWidth + "x" + patchHeight);
	}

	public int getPatchWidth() {
		return patchWidth;
	}

	public int getPatchHeight() {
		return patchHeight;
	}

	public Image getPatchedImage(int resolutionWidth, int resolutionHeight) {
		Image patchedImage = new Image(Display.getCurrent(),
				(patchWidth * 2) + resolutionWidth,
				(patchHeight * 2) + resolutionHeight);

		GC gc = new GC(patchedImage);

		/* top side */
		gc.drawImage(imageLT, 0, 0);
		gc.drawImage(imageCT, 0, 0, 1, patchHeight,
				patchWidth, 0, resolutionWidth, patchHeight);
		gc.drawImage(imageRT, patchWidth + resolutionWidth, 0);

		/* middle side */
		gc.drawImage(imageLC, 0, 0, patchWidth, 1,
				0, patchHeight, patchWidth, resolutionHeight);
		gc.drawImage(imageRC, 0, 0, patchWidth, 1,
				patchWidth + resolutionWidth, patchHeight, patchWidth, resolutionHeight);

		/* bottom side */
		gc.drawImage(imageLB, 0, patchHeight + resolutionHeight);
		gc.drawImage(imageCB, 0, 0, 1, patchHeight,
				patchWidth, patchHeight + resolutionHeight, resolutionWidth, patchHeight);
		gc.drawImage(imageRB, patchWidth + resolutionWidth, patchHeight + resolutionHeight);

		gc.dispose();

		return patchedImage;
	}

	private void loadPatches(String path) {
		ClassLoader loader = this.getClass().getClassLoader();

		imageLT = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameLT));
		logger.info("left-top image is loaded from " + path + filenameLT);
		imageCT = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameCT));
		imageRT = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameRT));

		imageLC = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameLC));
		imageRC = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameRC));

		imageLB = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameLB));
		imageCB = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameCB));
		imageRB = new Image(Display.getCurrent(),
				loader.getResourceAsStream(path + filenameRB));
	}

	public void freePatches() {
		imageLT.dispose();
		imageCT.dispose();
		imageRT.dispose();

		imageLC.dispose();
		imageRC.dispose();

		imageLB.dispose();
		imageCB.dispose();
		imageRB.dispose();
	}
}
