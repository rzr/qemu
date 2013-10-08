/**
 * Image Resource Management
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.config.EmulatorConfig;
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
	public static final String ICONS_FOLDER = "icons";
	public static final String IMAGES_FOLDER = "images";

	private static Logger logger =
			SkinLogger.getSkinLogger(ImageRegistry.class).getLogger();

	public enum ImageType {
		IMG_TYPE_MAIN,
		IMG_TYPE_PRESSED
	}

	public enum ResourceImageName {
		RESOURCE_ABOUT("about_Tizen_SDK.png"),
		RESOURCE_BLANK_GUIDE("blank-guide.png");

		private String name;

		private ResourceImageName(String name) {
			this.name = name;
		}

		public String getName() {
			return this.name;
		}
	}

	public enum IconName {
		DETAIL_INFO("detail_info.png"),
		ROTATE("rotate.png"),
		SCALE("scale.png"),
		SHELL("shell.png"),
		ECP("ecp.png"),
		ADVANCED("advanced.png"),
		CLOSE("close.png"),
		SCREENSHOT("screenshot.png"),
		USB_KEYBOARD("usb_keyboard.png"),
		HOST_KEYBOARD("host_keyboard.png"),
		DIAGNOSIS("diagnosis.png"),
		FORCE_CLOSE("force_close.png"),
		ABOUT("about.png"),

		COPY_SCREEN_SHOT("copy_screenshot_dialog.png"),
		REFRESH_SCREEN_SHOT("refresh_screenshot_dialog.png"),
		INCREASE_SCALE("increase_scale.png"),
		DECREASE_SCALE("decrease_scale.png"),
		SAVE_SCREEN_SHOT("save_screenshot_dialog.png"),

		EMULATOR_TITLE("emulator_icon.png"),
		EMULATOR_TITLE_ICO("emulator_icon.ico");
		
		private String name;
		
		private IconName(String name) {
			this.name = name;
		}
		
		public String getName() {
			return this.name;
		}
	}

	private Display display;
	private EmulatorUI dbiContents;

	private Map<String, Image> resourceImageMap;
	private Map<String, Image> iconImageMap;
	private Map<String, Image> skinImageMap;

	private String skinPath;

	private static ImageRegistry instance;
	private static boolean isInitialized;

	/**
	 *  Constructor
	 */
	private ImageRegistry() {
		/* do nothing */
	}

	public static ImageRegistry getInstance() {
		if (null == instance) {
			instance = new ImageRegistry();
		}

		return instance;
	}

	public void initialize(EmulatorConfig config, String skinPath) {
		if (isInitialized) {
			return;
		}
		isInitialized = true;

		this.display = Display.getDefault();

		this.skinPath = skinPath;
		this.dbiContents = config.getDbiContents();

		this.resourceImageMap = new HashMap<String, Image>();
		this.iconImageMap = new HashMap<String, Image>();
		this.skinImageMap = new HashMap<String, Image>();

		init(this.skinPath);
	}

	private void init(String argSkinPath) {
		RotationsType rotations = dbiContents.getRotations();

		if (null == rotations) {
			logger.severe("Fail to loading rotations element from dbi.");
			return;
		}

		List<RotationType> rotationList = rotations.getRotation();

		for (RotationType rotation : rotationList) {
			SkinRotation.put(rotation);
		}
	}

	private String makeKey(Short id, ImageType imageType) {
		return id + ":" + imageType.ordinal();
	}

	public Image getSkinImage(Short id, ImageType imageType) {
		Image image = skinImageMap.get(makeKey(id, imageType));

		if (image == null) {
			RotationsType rotations = dbiContents.getRotations();

			if (null == rotations) {
				logger.severe("Fail to loading rotations element from dbi.");
				return null;
			}

			logger.info("get skin image from " + skinPath);

			RotationType targetRotation = SkinRotation.getRotation(id);
			List<RotationType> rotationList = rotations.getRotation();

			for (RotationType rotation : rotationList) {
				ImageListType imageList = rotation.getImageList();
				if (imageList == null) {
					continue;
				}

				String mainImage = imageList.getMainImage();
				String keyPressedImage = imageList.getKeyPressedImage();

				if (targetRotation.getName().value().equals(rotation.getName().value())) {
					String mainKey = makeKey(id, ImageType.IMG_TYPE_MAIN);
					skinImageMap.put(mainKey,
							new Image(display, skinPath + File.separator + mainImage));

					String pressedKey = makeKey(id, ImageType.IMG_TYPE_PRESSED);
					skinImageMap.put(pressedKey,
							new Image(display, skinPath + File.separator + keyPressedImage));

					break;
				}
			}

			image = skinImageMap.get(makeKey(id, imageType));
		}

		return image;
	}

	public Image getResourceImage(ResourceImageName name) {
		String imageName = name.getName();
		Image image = resourceImageMap.get(imageName);

		if (image == null) {
			ClassLoader classLoader = this.getClass().getClassLoader();
			InputStream is = null;

			String resourcePath = IMAGES_FOLDER + "/" + imageName;

			try {
				is = classLoader.getResourceAsStream(resourcePath);
				if (null != is) {
					logger.info("resource image is loaded");
					resourceImageMap.put(imageName, new Image(display, is));
				} else {
					logger.severe("missing image : " + resourcePath);
				}
			} finally {
				IOUtil.close(is);
			}

			image = resourceImageMap.get(imageName);
			if (image != null) {
				logger.info("resource " + imageName + " size : " +
						image.getImageData().width + "x" +
						image.getImageData().height);
			}
		}

		return image;
	}

	public Image getIcon(IconName name) {
		if (iconImageMap.size() == 0) {
			/* load all of the icons at once */
			ClassLoader classLoader = this.getClass().getClassLoader();
			InputStream is = null;
			String imageName, imagePath;

			IconName[] values = IconName.values();
			for (IconName iconName : values) {
				imageName = iconName.getName();
				imagePath = ICONS_FOLDER + "/" + imageName;

				try {
					is = classLoader.getResourceAsStream(imagePath);
					if (null != is) {
						logger.fine("icon is loaded : " + imagePath);
						iconImageMap.put(imageName, new Image(display, is));
					} else {
						logger.severe("missing icon : " + imagePath);
					}
				} finally {
					IOUtil.close(is);
				}
			}
		}

		return iconImageMap.get(name.getName());
	}

	public void dispose() {
		Collection<Image> images = null;
		Iterator<Image> imageIterator = null;
		Image image = null;

		/* resource */
		if (null != resourceImageMap) {
			images = resourceImageMap.values();

			imageIterator = images.iterator();

			while (imageIterator.hasNext()) {
				image = imageIterator.next();
				image.dispose();
			}
		}

		/* icon */
		if (null != iconImageMap) {
			images = iconImageMap.values();

			imageIterator = images.iterator();

			while (imageIterator.hasNext()) {
				image = imageIterator.next();
				image.dispose();
			}
		}

		/* skin image */
		if (null != skinImageMap) {
			images = skinImageMap.values();

			imageIterator = images.iterator();

			while (imageIterator.hasNext()) {
				image = imageIterator.next();
				image.dispose();
			}
		}
	}
}
