/**
 * Capture a screenshot of the Emulator framebuffer
 *
 * Copyright ( C ) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or ( at your option ) any later version.
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

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorShmSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.exception.ScreenShotException;

public class ShmScreenShotWindow extends ScreenShotDialog {

	/**
	 * @brief constructor
	 * @param Image icon : screenshot window icon resource
	*/
	public ShmScreenShotWindow(Shell parent,
			SocketCommunicator communicator, EmulatorShmSkin emulatorSkin,
			EmulatorConfig config, Image icon) throws ScreenShotException {
		super(parent, communicator, emulatorSkin, config, icon);
	}

	protected void capture() throws ScreenShotException {
		int width = config.getArgInt(ArgsConstants.RESOLUTION_WIDTH);
		int height = config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT);

		int[] array = new int[width * height];
		int result = ((EmulatorShmSkin)emulatorSkin).getPixels(array); //from shared memory
		//logger.info("getPixels navtive function returned " + result);

		ImageData imageData = new ImageData(width, height, COLOR_DEPTH, paletteData2);
		for (int i = 0; i < height; i++) {
			imageData.setPixels(0, i, width, array, i * width);
		 }

		RotationInfo rotation = getCurrentRotation();
		imageData = rotateImageData(imageData, rotation);

		if (image != null) {
			image.dispose();
		}
		image = new Image(Display.getDefault(), imageData);
		imageCanvas.redraw();
	}
}