/**
 * Image Resource Management For General Key Window
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

public class GeneralKeyWindowImageRegistry {
	public static final String KEYWINDOW_FOLDER = "key-window";

	private static Logger logger = SkinLogger.getSkinLogger(
			GeneralKeyWindowImageRegistry.class).getLogger();

	public enum GeneralKeyWindowImageName {
		KEYWINDOW_PATCH_LT("LT.png"),
		KEYWINDOW_PATCH_T("T.png"),
		KEYWINDOW_PATCH_RT("RT.png"),
		KEYWINDOW_PATCH_L("L.png"),
		KEYWINDOW_PATCH_R("R.png"),
		KEYWINDOW_PATCH_LB("LB.png"),
		KEYWINDOW_PATCH_B("B.png"),
		KEYWINDOW_PATCH_RB("RB.png"),

		KEYBUTTON_NORMAL("keybutton_nml.png"),
		KEYBUTTON_HOVER("keybutton_hover.png"),
		KEYBUTTON_PUSHED("keybutton_pushed.png"),

		SCROLL_UPBUTTON_NORMAL("scroll_button_up_nml.png"),
		SCROLL_UPBUTTON_HOVER("scroll_button_up_hover.png"),
		SCROLL_UPBUTTON_PUSHED("scroll_button_up_pushed.png"),
		SCROLL_DOWNBUTTON_NORMAL("scroll_button_down_nml.png"),
		SCROLL_DOWNBUTTON_HOVER("scroll_button_down_hover.png"),
		SCROLL_DOWNBUTTON_PUSHED("scroll_button_down_pushed.png"),
		SCROLL_THUMB("scroll_thumb.png"),
		SCROLL_SHAFT("scroll_back.png");

		private String name;

		private GeneralKeyWindowImageName(String name) {
			this.name = name;
		}

		public String getName() {
			return this.name;
		}
	}

	private Display display;
	private Map<String, Image> keyWindowImageMap;

	/**
	 *  Constructor
	 */
	public GeneralKeyWindowImageRegistry(Display display) {
		this.display = display;
		this.keyWindowImageMap = new HashMap<String, Image>();
	}

	public Image getKeyWindowImage(GeneralKeyWindowImageName name) {
		if (keyWindowImageMap.size() == 0) {
			/* load all of the images at once */
			ClassLoader classLoader = this.getClass().getClassLoader();
			InputStream is = null;
			String imageName, imagePath;

			GeneralKeyWindowImageName[] values = GeneralKeyWindowImageName.values();
			for (GeneralKeyWindowImageName value : values) {
				imageName = value.getName();
				imagePath = ImageRegistry.IMAGES_FOLDER + "/"
						+ KEYWINDOW_FOLDER + "/" + imageName;

				try {
					is = classLoader.getResourceAsStream(imagePath);
					if (null != is) {
						logger.fine("KeyWindow image is loaded : " + imagePath);
						keyWindowImageMap.put(imageName, new Image(display, is));
					} else {
						logger.severe("missing image : " + imagePath);
					}
				} finally {
					IOUtil.close(is);
				}
			}
		}

		return keyWindowImageMap.get(name.getName());
	}

	public void dispose() {
		logger.info("dispose");

		if (null != keyWindowImageMap) {
			Image image = null;

			Collection<Image> images = keyWindowImageMap.values();
			Iterator<Image> imageIterator = images.iterator();

			while (imageIterator.hasNext()) {
				image = imageIterator.next();
				if (image != null) {
					image.dispose();
				}
			}
		}
	}
}
