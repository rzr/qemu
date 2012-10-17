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

import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.EmulatorSdlSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator.DataTranfer;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.log.SkinLogger;

public class SdlScreenShotWindow extends ScreenShotDialog {
	private Logger logger = SkinLogger.getSkinLogger(
			SdlScreenShotWindow.class).getLogger();

	/**
	 * @brief constructor
	 * @param Image icon : screenshot window icon resource
	*/
	public SdlScreenShotWindow(Shell parent,
			SocketCommunicator communicator, EmulatorSdlSkin emulatorSkin,
			EmulatorConfig config, Image icon) throws ScreenShotException {
		super(parent, communicator, emulatorSkin, config, icon);
	}
	
	protected void capture() throws ScreenShotException {
		DataTranfer dataTranfer = communicator.sendToQEMU( SendCommand.SCREEN_SHOT, null, true );
		byte[] receivedData = communicator.getReceivedData( dataTranfer );

		if ( null != receivedData ) {

			if ( null != this.image ) {
				this.image.dispose();
			}

			int width = config.getArgInt( ArgsConstants.RESOLUTION_WIDTH );
			int height = config.getArgInt( ArgsConstants.RESOLUTION_HEIGHT );
			ImageData imageData = new ImageData( width , height, COLOR_DEPTH, paletteData, 1, receivedData );
			
			RotationInfo rotation = getCurrentRotation();
			imageData = rotateImageData( imageData, rotation );

		 this.image = new Image( Display.getDefault(), imageData );
		 
		 imageCanvas.redraw();
			
		} else {
			throw new ScreenShotException( "Fail to get image data." );
		}
	}
}