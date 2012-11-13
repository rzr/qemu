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

package org.tizen.emulator.skin.image;

import java.io.File;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
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

	public static final String SKINS_FOLDER = "skins";
	public static final String GENERAL_FOLDER = "emul-general";
	public static final String ICON_FOLDER = "icons";
	
	public enum ImageType {
		IMG_TYPE_MAIN,
		IMG_TYPE_PRESSED
	}
	
	public enum IconName {
		
		DETAIL_INFO( "detail_info.png" ),
		ROTATE( "rotate.png" ),
		SCALE( "scale.png" ),
		SHELL( "shell.png" ),
		ADVANCED( "advanced.png" ),
		CLOSE( "close.png" ),
		SCREENSHOT( "screenshot.png" ),
		USB_KEYBOARD( "usb_keyboard.png" ),
		HOST_KEYBOARD( "host_keyboard.png" ),
		DIAGNOSIS( "diagnosis.png" ),
		ABOUT( "about.png" ),

		COPY_SCREEN_SHOT( "copy_screenshot_dialog.png" ),
		REFRESH_SCREEN_SHOT( "refresh_screenshot_dialog.png" ),
		INCREASE_SCALE( "increase_scale.png" ),
		DECREASE_SCALE( "decrease_scale.png" ),
		SAVE_SCREEN_SHOT( "save_screenshot_dialog.png" ),

		EMULATOR_TITLE( "emulator_icon.png" ),
		EMULATOR_TITLE_ICO( "emulator_icon.ico" );
		
		private String name;
		
		private IconName( String name ) {
			this.name = name;
		}
		
		public String getName() {
			return this.name;
		}
		
	}
	
	private Display display;
	private EmulatorUI dbiContents;

	private Map<String, Image> skinImageMap;
	private Map<String, Image> iconMap;

	private String argSkinPath;

	private static ImageRegistry instance;
	private static boolean isInitialized;

	private ImageRegistry() {
	}

	public static ImageRegistry getInstance() {
		if ( null == instance ) {
			instance = new ImageRegistry();
		}
		return instance;
	}

	public void initialize(EmulatorConfig config) {
		if (isInitialized) {
			return;
		}
		isInitialized = true;

		this.display = Display.getDefault();

		this.argSkinPath = config.getArg(ArgsConstants.SKIN_PATH);
		this.dbiContents = config.getDbiContents();
		this.skinImageMap = new HashMap<String, Image>();
		this.iconMap = new HashMap<String, Image>();

		init( this.argSkinPath );

	}

	public static String getSkinPath(String argSkinPath) {
		/* When emulator has a invalid skin path,
		 emulator uses default skin path instead of it */
		String defaultSkinPath = ".." + //TODO:
				File.separator + SKINS_FOLDER + File.separator + GENERAL_FOLDER;

		if (argSkinPath == null) {
			return defaultSkinPath;
		}

		File f = new File(argSkinPath);
		if (f.isDirectory() == false) {
			return defaultSkinPath;
		}

		return argSkinPath;
	}

	private void init( String argSkinPath ) {

		RotationsType rotations = dbiContents.getRotations();

		if ( null == rotations ) {
			logger.severe( "Fail to loading rotations element from dbi." );
			return;
		}

		List<RotationType> rotationList = rotations.getRotation();

		for ( RotationType rotation : rotationList ) {
			SkinRotation.put( rotation );
		}

	}

	public ImageData getSkinImageData( Short id, ImageType imageType ) {

		Image image = skinImageMap.get( makeKey( id, imageType ) );

		if ( null != image ) {

			return image.getImageData();

		} else {

			RotationsType rotations = dbiContents.getRotations();

			if ( null == rotations ) {
				logger.severe( "Fail to loading rotations element from dbi." );
				return null;
			}

			String skinPath = getSkinPath(argSkinPath);
			logger.info("get image data of skin from " + skinPath);

			RotationType targetRotation = SkinRotation.getRotation( id );

			List<RotationType> rotationList = rotations.getRotation();

			for ( RotationType rotation : rotationList ) {

				ImageListType imageList = rotation.getImageList();
				String mainImage = imageList.getMainImage();
				String keyPressedImage = imageList.getKeyPressedImage();

				if ( targetRotation.getName().value().equals( rotation.getName().value() ) ) {

					String mainKey = makeKey( id, ImageType.IMG_TYPE_MAIN );
					skinImageMap.put( mainKey, new Image( display, skinPath + File.separator + mainImage ) );

					String pressedKey = makeKey( id, ImageType.IMG_TYPE_PRESSED );
					skinImageMap.put( pressedKey, new Image( display, skinPath + File.separator + keyPressedImage ) );

					break;

				}

			}

			Image registeredImage = skinImageMap.get( makeKey( id, imageType ) );

			if ( null != registeredImage ) {
				return registeredImage.getImageData();
			} else {
				return null;
			}

		}
	}

	private String makeKey( Short id, ImageType imageType ) {
		return id + ":" + imageType.ordinal();
	}

	public Image getIcon( IconName name ) {

		if ( 0 != iconMap.size() ) {

			Image image = iconMap.get( name.getName() );
			return image;

		} else {

			// load all of the icons at once.

			ClassLoader classLoader = this.getClass().getClassLoader();
			IconName[] values = IconName.values();

			for ( IconName iconName : values ) {

				String icoNname = iconName.getName();

				String iconPath = ICON_FOLDER + "/" + icoNname;

				InputStream is = null;
				try {
					is = classLoader.getResourceAsStream( iconPath );
					if ( null != is ) {
						logger.fine( "load icon:" + iconPath );
						iconMap.put( icoNname, new Image( display, is ) );
					} else {
						logger.severe( "missing icon:" + iconPath );
					}
				} finally {
					IOUtil.close( is );
				}

			}

			return iconMap.get( name.getName() );

		}

	}

	public void dispose() {

		if ( null != skinImageMap ) {

			Collection<Image> images = skinImageMap.values();

			Iterator<Image> imageIterator = images.iterator();

			while ( imageIterator.hasNext() ) {
				Image image = imageIterator.next();
				image.dispose();
			}

		}

		if ( null != iconMap ) {

			Collection<Image> icons = iconMap.values();

			Iterator<Image> iconIterator = icons.iterator();

			while ( iconIterator.hasNext() ) {
				Image image = iconIterator.next();
				image.dispose();
			}

		}

	}
	
}
