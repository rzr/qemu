/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
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

package org.tizen.emulator.skin.util;

import java.io.File;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Region;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.Scale;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.dbi.EventInfoType;
import org.tizen.emulator.skin.dbi.KeyMapType;
import org.tizen.emulator.skin.dbi.LcdType;
import org.tizen.emulator.skin.dbi.RegionType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.ImageType;


/**
 * 
 *
 */
public class SkinUtil {

	public static final int UNKNOWN_KEYCODE = -1;
	public static final int SCALE_CONVERTER = 100;
	public static final String EMULATOR_PREFIX = "emulator";

	private SkinUtil() {
	}

	public static String getVmName( EmulatorConfig config ) {
		
		String vmPath = config.getArg( ArgsConstants.VM_PATH );

		String regex = "";
		if ( SwtUtil.isWindowsPlatform() ) {
			regex = "\\" + File.separator;
		} else {
			regex = File.separator;
		}
		String[] split = StringUtil.nvl( vmPath ).split( regex );
		String vmName = split[split.length - 1];

		return vmName;
		
	}
	
	public static String makeEmulatorName( EmulatorConfig config ) {

		String vmName = getVmName( config );

		if ( StringUtil.isEmpty( vmName ) ) {
			vmName = EMULATOR_PREFIX;
		}

		int portNumber = config.getArgInt( ArgsConstants.NET_BASE_PORT );
		return vmName + ":" + portNumber;

	}

	public static String getSdbPath() {
		String sdbPath = null;

		if (SwtUtil.isWindowsPlatform()) {
			sdbPath = ".\\..\\..\\sdb-run.bat";
		} else {
			sdbPath = "./../../sdb";
		}

		return sdbPath;
	}

	public static void adjustLcdGeometry( Canvas lcdCanvas, int scale, short rotationId ) {

		RotationType rotation = SkinRotation.getRotation( rotationId );

		LcdType lcd = rotation.getLcd();
		RegionType region = lcd.getRegion();

		Integer left = region.getLeft();
		Integer top = region.getTop();
		Integer width = region.getWidth();
		Integer height = region.getHeight();

		float convertedScale = convertScale( scale );

		int l = (int) ( left * convertedScale );
		int t = (int) ( top * convertedScale );
		int w = (int) ( width * convertedScale );
		int h = (int) ( height * convertedScale );

		lcdCanvas.setBounds( l, t, w, h );

	}

	public static SkinRegion getHardKeyArea( int currentX, int currentY, short rotationId, int scale ) {

		float convertedScale = convertScale( scale );

		RotationType rotation = SkinRotation.getRotation( rotationId );

		List<KeyMapType> keyMapList = rotation.getKeyMapList().getKeyMap();

		for ( KeyMapType keyMap : keyMapList ) {

			RegionType region = keyMap.getRegion();

			int scaledX = (int) ( region.getLeft() * convertedScale );
			int scaledY = (int) ( region.getTop() * convertedScale );
			int scaledWidth = (int) ( region.getWidth() * convertedScale );
			int scaledHeight = (int) ( region.getHeight() * convertedScale );

			if ( isInGeometry( currentX, currentY, scaledX, scaledY, scaledWidth, scaledHeight ) ) {
				return new SkinRegion( scaledX, scaledY, scaledWidth, scaledHeight );
			}

		}

		return null;

	}

	public static int getHardKeyCode( int currentX, int currentY, short rotationId, int scale ) {

		float convertedScale = convertScale( scale );

		RotationType rotation = SkinRotation.getRotation( rotationId );

		List<KeyMapType> keyMapList = rotation.getKeyMapList().getKeyMap();

		for ( KeyMapType keyMap : keyMapList ) {
			RegionType region = keyMap.getRegion();

			int scaledX = (int) ( region.getLeft() * convertedScale );
			int scaledY = (int) ( region.getTop() * convertedScale );
			int scaledWidth = (int) ( region.getWidth() * convertedScale );
			int scaledHeight = (int) ( region.getHeight() * convertedScale );

			if ( isInGeometry( currentX, currentY, scaledX, scaledY, scaledWidth, scaledHeight ) ) {
				EventInfoType eventInfo = keyMap.getEventInfo();
				return eventInfo.getKeyCode();
			}
		}

		return UNKNOWN_KEYCODE;

	}

	//TODO: HardKey object
	public static String getHardKeyToolTip( int currentX, int currentY, short rotationId, int scale ) {

		float convertedScale = convertScale( scale );

		RotationType rotation = SkinRotation.getRotation( rotationId );

		List<KeyMapType> keyMapList = rotation.getKeyMapList().getKeyMap();

		for ( KeyMapType keyMap : keyMapList ) {
			RegionType region = keyMap.getRegion();

			int scaledX = (int) ( region.getLeft() * convertedScale );
			int scaledY = (int) ( region.getTop() * convertedScale );
			int scaledWidth = (int) ( region.getWidth() * convertedScale );
			int scaledHeight = (int) ( region.getHeight() * convertedScale );

			if ( isInGeometry( currentX, currentY, scaledX, scaledY, scaledWidth, scaledHeight ) ) {
				return keyMap.getTooltip();
			}
		}

		return null;

	}

	public static boolean isInGeometry( int currentX, int currentY, int targetX, int targetY, int targetWidth,
			int targetHeight ) {

		if ( ( currentX >= targetX ) && ( currentY >= targetY ) ) {
			if ( ( currentX <= ( targetX + targetWidth ) ) && ( currentY <= ( targetY + targetHeight ) ) ) {
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

	public static void trimShell(Shell shell, Image image, int left, int top,
		int width, int height) {

		if (null == image) {
			return;
		}

		ImageData imageData = image.getImageData();

		int right = left + width;
		int bottom = top + height;

		Region region = shell.getRegion();
		if (region == null) {
			return;
		}

		for (int i = left; i < right; i++) {
			for (int j = top; j < bottom; j++) {
				int alpha = imageData.getAlpha(i, j);
				if (0 == alpha) {
					region.subtract(i, j, 1, 1);
				}
			}
		}

		shell.setRegion(region);
	}

	public static int[] convertMouseGeometry( int originalX, int originalY, int lcdWidth, int lcdHeight, int scale,
			int angle ) {

		float convertedScale = convertScale( scale );

		int x = (int) ( originalX * ( 1 / convertedScale ) );
		int y = (int) ( originalY * ( 1 / convertedScale ) );

		int rotatedX = x;
		int rotatedY = y;

		if ( RotationInfo.LANDSCAPE.angle() == angle ) {
			rotatedX = lcdWidth - y;
			rotatedY = x;
		} else if ( RotationInfo.REVERSE_PORTRAIT.angle() == angle ) {
			rotatedX = lcdWidth - x;
			rotatedY = lcdHeight - y;
		} else if ( RotationInfo.REVERSE_LANDSCAPE.angle() == angle ) {
			rotatedX = y;
			rotatedY = lcdHeight - x;
		}

		return new int[] { rotatedX, rotatedY };

	}

	public static Image createScaledImage( ImageRegistry imageRegistry, Shell shell, short rotationId, int scale,
			ImageType type ) {

		ImageData originalImageData = imageRegistry.getSkinImageData( rotationId, type );

		if ( null == originalImageData ) {
			return null;
		}

		ImageData imageData = (ImageData) originalImageData.clone();

		float convertedScale = convertScale( scale );

		int width = (int) ( originalImageData.width * convertedScale );
		int height = (int) ( originalImageData.height * convertedScale );
		imageData = imageData.scaledTo( width, height );

		Image image = new Image( shell.getDisplay(), imageData );
		return image;

	}

	public static float convertScale( int scale ) {
		return (float) scale / SCALE_CONVERTER;
	}

	public static int getValidScale( EmulatorConfig config ) {

		int storedScale = config.getSkinPropertyInt( SkinPropertiesConstants.WINDOW_SCALE,
				EmulatorConfig.DEFAULT_WINDOW_SCALE );

		if ( !SkinUtil.isValidScale( storedScale ) ) {
			return EmulatorConfig.DEFAULT_WINDOW_SCALE;
		}else {
			return storedScale;
		}
		
	}

	public static boolean isValidScale( int scale ) {
		if ( Scale.SCALE_100.value() == scale || Scale.SCALE_75.value() == scale
		|| Scale.SCALE_50.value() == scale || Scale.SCALE_25.value() == scale ) {
			return true;
		} else {
			return false;
		}
	}
	
	public static <T> void openMessage( Shell shell, String title, String message, int style, EmulatorConfig config ) {
		
		MessageBox messageBox = new MessageBox( shell, style );
		
		if( !StringUtil.isEmpty( title ) ) {
			messageBox.setText( title );
		}else {
			messageBox.setText( makeEmulatorName( config ) );
		}
		
		messageBox.setMessage( StringUtil.nvl( message ) );
		messageBox.open();
		
	}

}
