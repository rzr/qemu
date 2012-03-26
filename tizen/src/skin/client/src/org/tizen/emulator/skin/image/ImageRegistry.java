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
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.dbi.EmulatorUI;
import org.tizen.emulator.skin.dbi.ImageListType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dbi.RotationsType;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.SkinRotation;


/**
 * 
 *
 */
public class ImageRegistry {
	
	private Logger logger = SkinLogger.getSkinLogger( ImageRegistry.class ).getLogger();

	public static final String SKIN_FOLDER = "skins";
	public static final String IMAGE_FOLDER_PREFIX = "emul_";
	public static final String ICON_FOLDER = "icons";
	
	public enum ImageType {
		IMG_TYPE_MAIN,
		IMG_TYPE_PRESSED
	}
	
	public enum IconName {
		
		DEVICE_INFO( "device_info.png" ),
		ROTATE( "rotate.png" ),
		SCALING( "scaling.png" ),
		SHELL( "shell.png" ),
		ADVANCED( "advanced.png" ),
		CLOSE( "close.png" ),
		SCREENSHOT( "screenshot.png" ),
		USB_KEBOARD( "keypad.png" ),
		ABOUT( "about.png" ),
		
		EMULATOR_TITLE( "Emulator_20x20.png" ),
		EMULATOR_TITLE_ICO( "Emulator.ico" );
		
		private String name;
		
		private IconName( String name ) {
			this.name = name;
		}
		
		public String getName() {
			return this.name;
		}
		
	}
	
	private Display display;
	private int resolutionWidth;
	private int resolutionHeight;
	private EmulatorUI dbiContents;
	
	private Map<String, Image> skinImageMap;
	private Map<String, Image> iconMap;
	
	public ImageRegistry(Display display, EmulatorConfig config ) {
		
		this.display = display;
		
		int lcdWidth = Integer.parseInt( config.getArg( ArgsConstants.RESOLUTION_WIDTH ) );
		int lcdHeight = Integer.parseInt( config.getArg( ArgsConstants.RESOLUTION_HEIGHT ) );
		String skinPath = (String) config.getArg( ArgsConstants.SKIN_PATH );

		this.resolutionWidth = lcdWidth;
		this.resolutionHeight = lcdHeight;
		this.dbiContents = config.getDbiContents();
		this.skinImageMap = new HashMap<String, Image>();
		this.iconMap = new HashMap<String, Image>();
		
		init(skinPath);
		
	}
	
	public static String getSkinPath( String argSkinPath, int lcdWidth, int lcdHeight ) {
		String skinPath = null;

		File f = new File(argSkinPath);
		if (argSkinPath == null || f.isDirectory() == false) {
			skinPath = ".." + File.separator + SKIN_FOLDER + File.separator + IMAGE_FOLDER_PREFIX + lcdWidth + "x" + lcdHeight;
			return skinPath;
		}

		return argSkinPath;
	}

	private void init(String argSkinPath) {

		String skinPath = getSkinPath( argSkinPath, resolutionWidth, resolutionHeight );
		
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
					skinImageMap.put( mainKey, new Image( display, skinPath + File.separator + mainImage ) );

					String pressedKey = makeKey( entry.getKey(), ImageType.IMG_TYPE_PRESSED );
					skinImageMap.put( pressedKey, new Image( display, skinPath + File.separator + keyPressedImage ) );

				}
			}

		}
		
		ClassLoader classLoader = this.getClass().getClassLoader();
		IconName[] values = IconName.values();
		
		for ( IconName iconName : values ) {
			
			String name = iconName.getName();
			
			String iconPath = ICON_FOLDER + "/" + name;
			
			InputStream is = null;
			try {
				is = classLoader.getResourceAsStream( iconPath );
				if( null != is ) {
					logger.info( "load icon:" + iconPath );
					iconMap.put( name, new Image( display, is ) );
				}else {
					logger.severe( "missing icon:" + iconPath );
				}
			} finally {
				IOUtil.close( is );
			}
			
		}

	}
	
	public ImageData getSkinImageData( Short id, ImageType imageType ) {
		Image image = skinImageMap.get( makeKey(id, imageType) );
		if( null != image ) {
			return image.getImageData();
		}else {
			return null;
		}
	}
	
	private String makeKey( Short id, ImageType imageType ) {
		return id + ":" + imageType.ordinal();
	}
	
	public Image getIcon( IconName name ) {
		return iconMap.get( name.getName() );
	}
	
	public void dispose() {

		Collection<Image> images = skinImageMap.values();

		Iterator<Image> imageIterator = images.iterator();

		while ( imageIterator.hasNext() ) {
			Image image = imageIterator.next();
			image.dispose();
		}

		Collection<Image> icons = iconMap.values();

		Iterator<Image> iconIterator = icons.iterator();

		while ( iconIterator.hasNext() ) {
			Image image = iconIterator.next();
			image.dispose();
		}

	}
	
}
