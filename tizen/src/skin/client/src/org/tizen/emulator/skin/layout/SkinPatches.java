/**
 * General Skin Frame Maker
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

package org.tizen.emulator.skin.layout;

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.log.SkinLogger;

public class SkinPatches {
	private Logger logger =
			SkinLogger.getSkinLogger(SkinPatches.class).getLogger();

	private Display display;
	private int patchWidth;
	private int patchHeight;

	/* nine patches */
	private Image imageLT; /* left-top */
	private Image imageT; /* middle-top */
	private Image imageRT; /* right-top */

	private Image imageL;
	private Image imageR;

	private Image imageLB; /* left-bottom */
	private Image imageB;
	private Image imageRB;

	/**
	 *  Constructor
	 */
	public SkinPatches(
			Image leftTop, Image middleTop, Image rightTop,
			Image leftMiddle, Image rightMiddle,
			Image leftBottom, Image middleBottom, Image rightBottom) {
		this.display = Display.getCurrent();

		imageLT = leftTop;
		imageT = middleTop;
		imageRT = rightTop;

		imageL = leftMiddle;
		imageR = rightMiddle;

		imageLB = leftBottom;
		imageB = middleBottom;
		imageRB = rightBottom;

		// TODO: configurable
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

	public Image getPatchedImage(int centerPatchWidth, int centerPatchHeight) {
		int patchedImageWidth = (patchWidth * 2) + centerPatchWidth;
		int patchedImageHeight = (patchHeight * 2) + centerPatchHeight;

		Image patchedImage = new Image(display,
				patchedImageWidth, patchedImageHeight);

		// TODO: copy alphaData
		GC gc = new GC(patchedImage);
		gc.setInterpolation(SWT.NONE);

		gc.setBackground(display.getSystemColor(SWT.COLOR_MAGENTA));
		gc.fillRectangle(0, 0, patchedImageWidth, patchedImageHeight);

		/* top side */
		gc.drawImage(imageLT, 0, 0);
		gc.drawImage(imageT, 0, 0, imageT.getImageData().width, imageT.getImageData().height,
				patchWidth, 0, centerPatchWidth, patchHeight);
		gc.drawImage(imageRT, patchWidth + centerPatchWidth, 0);

		/* middle side */
		gc.drawImage(imageL, 0, 0, imageL.getImageData().width, imageL.getImageData().height,
				0, patchHeight, patchWidth, centerPatchHeight);
		gc.setBackground(new Color(display, new RGB(38, 38, 38)));
		gc.fillRectangle(patchWidth, patchHeight, centerPatchWidth, centerPatchHeight); /* center */
		gc.drawImage(imageR, 0, 0, imageR.getImageData().width, imageR.getImageData().height,
				patchWidth + centerPatchWidth, patchHeight, patchWidth, centerPatchHeight);

		/* bottom side */
		gc.drawImage(imageLB, 0, patchHeight + centerPatchHeight);
		gc.drawImage(imageB, 0, 0, imageB.getImageData().width, imageB.getImageData().height,
				patchWidth, patchHeight + centerPatchHeight, centerPatchWidth, patchHeight);
		gc.drawImage(imageRB, patchWidth + centerPatchWidth, patchHeight + centerPatchHeight);

		gc.dispose();

		return patchedImage;
	}
}
