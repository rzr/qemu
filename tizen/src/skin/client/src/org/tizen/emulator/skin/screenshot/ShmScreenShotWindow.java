/**
 * Capture a screenshot of the Emulator framebuffer
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

package org.tizen.emulator.skin.screenshot;

import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorShmSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.log.SkinLogger;

public class ShmScreenShotWindow extends ScreenShotDialog {
	private static Logger logger = SkinLogger.getSkinLogger(
			ShmScreenShotWindow.class).getLogger();

	/**
	 * @brief constructor
	 * @param Image icon : screenshot window icon resource
	*/
	public ShmScreenShotWindow(Shell parent,
			EmulatorShmSkin emulatorSkin, EmulatorConfig config,
			Image icon) throws ScreenShotException {
		super(parent, emulatorSkin, config, icon);
	}

	protected void capture() throws ScreenShotException {
		logger.info("screenshot capture");

		int width = config.getArgInt(ArgsConstants.RESOLUTION_WIDTH);
		int height = config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT);

		int[] arrayFramebuffer = new int[width * height];
		int result =
				((EmulatorShmSkin) emulatorSkin).getPixels(arrayFramebuffer);
		//logger.info("getPixels native function returned " + result);

		ImageData imageData = new ImageData(width, height, 24, paletteData_ARGB);
		/* from shared memory */
		imageData.setPixels(0, 0,
				width * height, arrayFramebuffer, 0);

		RotationInfo rotation = getCurrentRotation();
		imageData = rotateImageData(imageData, rotation);

		Image tempImage = image;
		image = new Image(Display.getDefault(), imageData);

		if (tempImage != null) {
			tempImage.dispose();
		}

		imageCanvas.redraw();
	}
}