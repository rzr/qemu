/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
 * Hyunjin Lee <hyunjin816.lee@samsung.com>
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

package org.tizen.emulator.skin.util;

import java.util.List;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Region;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.dbi.EventInfoType;
import org.tizen.emulator.skin.dbi.KeyMapListType;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.dbi.RegionType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.layout.HWKey;
import org.tizen.emulator.skin.log.SkinLogger;


/**
 * 
 *
 */
public class SpecialKeyWindowUtil {
	public static final int UNKNOWN_KEYCODE = -1;
	public static final int TOUCH_KEYCODE = -2;
	
	private static Logger logger =
			SkinLogger.getSkinLogger(SkinUtil.class).getLogger();

	private SpecialKeyWindowUtil() {
		/* do nothing */
	}

	public static List<KeyMapType> getHWKeyMapList(short rotationId) {
		RotationType rotation = KeyWindowRotation.getRotation(rotationId);
		if (rotation == null) {
			return null;
		}

		KeyMapListType list = rotation.getKeyMapList();
		if (list == null) {
			/* try to using a KeyMapList of portrait */
			rotation = KeyWindowRotation.getRotation(RotationInfo.PORTRAIT.id());
			if (rotation == null) {
				return null;
			}

			list = rotation.getKeyMapList();
			if (list == null) {
				return null;
			}
		}

		return list.getKeyMap();
	}

	public static HWKey getHWKey(
			int currentX, int currentY, short rotationId) {		

		List<KeyMapType> keyMapList = getHWKeyMapList(rotationId);
		if (keyMapList == null) {
			return null;
		}

		for (KeyMapType keyMap : keyMapList) {
			RegionType region = keyMap.getRegion();

			int scaledX = (int) region.getLeft();
			int scaledY = (int) region.getTop();
			int scaledWidth = (int) region.getWidth();
			int scaledHeight = (int) region.getHeight();

			if (isInGeometry(currentX, currentY, scaledX, scaledY, scaledWidth, scaledHeight)) {
				EventInfoType eventInfo = keyMap.getEventInfo();

				HWKey hwKey = new HWKey();
				hwKey.setKeyCode(eventInfo.getKeyCode());
				hwKey.setRegion(new SkinRegion(scaledX, scaledY, scaledWidth, scaledHeight));
				hwKey.setTooltip(keyMap.getTooltip());

				return hwKey;
			}
		}

		return null;
	}

	public static boolean isInGeometry(int currentX, int currentY,
			int targetX, int targetY, int targetWidth, int targetHeight) {

		if ((currentX >= targetX) && (currentY >= targetY)) {
			if ((currentX <= (targetX + targetWidth)) &&
					(currentY <= (targetY + targetHeight))) {
				return true;
			}
		}

		return false;
	}	
	
	public static void trimShell( Shell shell, Image image ) {

		// trim transparent pixels in image. especially, corner round areas.

		if ( null == image ) {
			return;
		}

		ImageData imageData = image.getImageData();

		int width = imageData.width;
		int height = imageData.height;

		Region region = new Region();
		region.add( new Rectangle( 0, 0, width, height ) );

		for ( int i = 0; i < width; i++ ) {
			for ( int j = 0; j < height; j++ ) {
				int alpha = imageData.getAlpha( i, j );
				if ( 0 == alpha ) {
					region.subtract( i, j, 1, 1 );
				}
			}
		}

		shell.setRegion( region );

	}
}
