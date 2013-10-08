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

import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;


/**
 * 
 *
 */
public class ImageRegistry {
	public static final String ICONS_FOLDER = "icons";
	public static final String IMAGES_FOLDER = "images";

	private static Logger logger =
			SkinLogger.getSkinLogger(ImageRegistry.class).getLogger();

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
	private Map<String, Image> resourceImageMap;
	private Map<String, Image> iconImageMap;

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
		this.resourceImageMap = new HashMap<String, Image>();
		this.iconImageMap = new HashMap<String, Image>();
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
	}
}
