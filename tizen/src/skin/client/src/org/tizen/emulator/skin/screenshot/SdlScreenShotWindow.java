/**
 * Screenshot Window
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
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.EmulatorSdlSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator.DataTranfer;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.log.SkinLogger;

public class SdlScreenShotWindow extends ScreenShotDialog {
	private static Logger logger = SkinLogger.getSkinLogger(
			SdlScreenShotWindow.class).getLogger();

	private PaletteData palette;

	/**
	 * @brief constructor
	 * @param Image icon : screenshot window icon resource
	*/
	public SdlScreenShotWindow(EmulatorSdlSkin emulatorSkin,
			EmulatorConfig config, PaletteData palette, Image icon) {
		super(emulatorSkin, config, icon);

		this.palette = palette;
	}

	protected void capture() throws ScreenShotException {
		logger.info("screenshot capture");

		DataTranfer dataTranfer = skin.communicator.sendDataToQEMU(
				SendCommand.SEND_SCREENSHOT_REQ, null, true);
		byte[] receivedData =
				skin.communicator.getReceivedData(dataTranfer);

		if (null != receivedData) {
			int width = skin.getEmulatorSkinState().getCurrentResolutionWidth();
			int height = skin.getEmulatorSkinState().getCurrentResolutionHeight();
			ImageData imageData = new ImageData(width, height,
					EmulatorSdlSkin.DISPLAY_COLOR_DEPTH, palette, 1, receivedData);

			RotationInfo rotation = getCurrentRotation();
			imageData = rotateImageData(imageData, rotation);

			Image tempImage = imageFrame;
			imageFrame = new Image(Display.getDefault(), imageData);

			if (tempImage != null) {
				tempImage.dispose();
			}

			canvasFrame.redraw();
		} else {
			throw new ScreenShotException("Fail to get image data.");
		}
	}
}