/**
 * Image Resource Management For General-Purpose Skin
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.image;

import java.io.File;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;

public class GeneralSkinImageRegistry {
	private static final String PATCH_IMAGES_PATH = "emul-window";

	private static Logger logger = SkinLogger.getSkinLogger(
			GeneralSkinImageRegistry.class).getLogger();

	public enum GeneralSkinImageName {
		TOGGLE_BUTTON_NORMAL("arrow_nml.png"),
		TOGGLE_BUTTON_HOVER("arrow_hover.png"),
		TOGGLE_BUTTON_PUSHED("arrow_pushed.png");

		private String name;

		private GeneralSkinImageName(String name) {
			this.name = name;
		}

		public String getName() {
			return this.name;
		}
	}

	private Display display;
	private Map<String, Image> skinImageMap;

	/**
	 *  Constructor
	 */
	public GeneralSkinImageRegistry(Display display) {
		this.display = display;
		this.skinImageMap = new HashMap<String, Image>();
	}

	public Image getSkinImage(GeneralSkinImageName name) {
		if (skinImageMap.size() == 0) {
			/* load all of the images at once */
			ClassLoader classLoader = this.getClass().getClassLoader();
			InputStream is = null;
			String imageName, imagePath;

			GeneralSkinImageName[] values = GeneralSkinImageName.values();
			for (GeneralSkinImageName value : values) {
				imageName = value.getName();
				imagePath = ImageRegistry.IMAGES_FOLDER + "/"
						+ PATCH_IMAGES_PATH + "/" + imageName;

				try {
					is = classLoader.getResourceAsStream(imagePath);
					if (null != is) {
						logger.fine("KeyWindow image is loaded : " + imagePath);
						skinImageMap.put(imageName, new Image(display, is));
					} else {
						logger.severe("missing image : " + imagePath);
					}
				} finally {
					IOUtil.close(is);
				}
			}
		}

		return skinImageMap.get(name.getName());
	}

	public void dispose() {
		if (null != skinImageMap) {
			Image image = null;

			Collection<Image>  images = skinImageMap.values();
			Iterator<Image> imageIterator = images.iterator();

			while (imageIterator.hasNext()) {
				image = imageIterator.next();
				image.dispose();
			}
		}
	}
}
