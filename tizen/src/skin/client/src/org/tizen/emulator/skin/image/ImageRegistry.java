/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

package org.tizen.emulator.skin.image;

import java.io.File;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.dbi.ImageListType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dbi.RotationsType;
import org.tizen.emulator.skin.util.SkinRotation;


/**
 * 
 *
 */
public class ImageRegistry {

	public static final String SKIN_FOLDER = "skins";
	public static final String IMAGE_FOLDER_PREFIX = "emul_";
	public static final String ICON_FOLDER = "icons";
	
	public enum ImageType {
		IMG_TYPE_MAIN,
		IMG_TYPE_PRESSED
	}
	
	public enum IconName {
		
	}
	
	private Display display;
	private int resolutionWidth;
	private int resolutionHeight;
	private EmulatorUI dbiContents;
	
	private Map<String, Image> imageMap;
	
	public ImageRegistry(Display display, EmulatorConfig config ) {
		
		this.display = display;
		
		int lcdWidth = Integer.parseInt( config.getArg( ArgsConstants.RESOLUTION_WIDTH ) );
		int lcdHeight = Integer.parseInt( config.getArg( ArgsConstants.RESOLUTION_HEIGHT ) );

		this.resolutionWidth = lcdWidth;
		this.resolutionHeight = lcdHeight;
		this.dbiContents = config.getDbiContents();
		this.imageMap = new HashMap<String, Image>();
		
		init();
		
	}
	
	public static String getSkinPath( int lcdWidth, int lcdHeight ) {
		String skinPath = ".." + File.separator + SKIN_FOLDER + File.separator + IMAGE_FOLDER_PREFIX + lcdWidth + "x" + lcdHeight;
		return skinPath;
	}
	
	private void init() {

		String skinPath = getSkinPath( resolutionWidth, resolutionHeight );
		
		RotationsType rotations = dbiContents.getRotations();
		List<RotationType> rotationList = rotations.getRotation();

		for ( RotationType rotation : rotationList ) {
			SkinRotation.put( rotation );
		}

		for ( RotationType rotation : rotationList ) {

			ImageListType imageList = rotation.getImageList();
			String mainImage = imageList.getMainImage();
			String keyPressedImage = imageList.getKeyPressedImage();

			Iterator<Entry<Short, RotationType>> rotationIterator = SkinRotation.getRotationIterator();

			while ( rotationIterator.hasNext() ) {

				Entry<Short, RotationType> entry = rotationIterator.next();

				if ( entry.getValue().getName().value().equals( rotation.getName().value() ) ) {

					String mainKey = makeKey( entry.getKey(), ImageType.IMG_TYPE_MAIN );
					imageMap.put( mainKey, new Image( display, skinPath + File.separator + mainImage ) );

					String pressedKey = makeKey( entry.getKey(), ImageType.IMG_TYPE_PRESSED );
					imageMap.put( pressedKey, new Image( display, skinPath + File.separator + keyPressedImage ) );

				}
			}

		}

	}
	
//	public Image getImage( Short id, ImageType imageType ) {
//		Image image = imageMap.get( makeKey(id, imageType) );
//		return image;
//	}
	
	public ImageData getImageData( Short id, ImageType imageType ) {
		Image image = imageMap.get( makeKey(id, imageType) );
		if( null != image ) {
			return image.getImageData();
		}else {
			return null;
		}
	}
	
	private String makeKey( Short id, ImageType imageType ) {
		return id + ":" + imageType.ordinal();
	}
	
	public void dispose() {
		
		Collection<Image> images = imageMap.values();
		
		Iterator<Image> iterator = images.iterator();
		
		while(iterator.hasNext()) {
			Image image = iterator.next();
			image.dispose();
		}
		
	}
	
}
