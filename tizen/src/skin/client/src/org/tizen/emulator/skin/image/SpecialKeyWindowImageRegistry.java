/**
 * Image Resource Management For Special Key Window
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
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.logging.Logger;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.keywindow.dbi.ImageListType;
import org.tizen.emulator.skin.keywindow.dbi.KeyWindowUI;
import org.tizen.emulator.skin.log.SkinLogger;


/**
 * 
 *
 */
public class SpecialKeyWindowImageRegistry {
	private static Logger logger =
			SkinLogger.getSkinLogger(SpecialKeyWindowImageRegistry.class).getLogger();

	public enum SpecailKeyWindowImageType {
		SPECIAL_IMAGE_TYPE_NORMAL,
		SPECIAL_IMAGE_TYPE_PRESSED
	}

	private Display display;
	private String imagePath;
	private KeyWindowUI dbiContents;
	private Map<String, Image> keyWindowImageMap;

	/**
	 *  Constructor
	 */
	public SpecialKeyWindowImageRegistry(
			Display display, KeyWindowUI dbiContents, String imagePath) {
		this.display = display;
		this.imagePath = imagePath;
		this.dbiContents = dbiContents;
		this.keyWindowImageMap = new HashMap<String, Image>();
	}

	private String makeKey(Short id, SpecailKeyWindowImageType imageType) {
		return id + ":" + imageType.ordinal();
	}

	public Image getKeyWindowImage(Short id, SpecailKeyWindowImageType imageType) {
		Image image = keyWindowImageMap.get(makeKey(id, imageType));

		if (image == null) {
			ImageListType imageList = dbiContents.getImageList();
			if (imageList == null) {
				return null;
			}

			logger.info("get Key Window image from " + imagePath);

			String mainImage = imageList.getMainImage();
			String keyPressedImage = imageList.getKeyPressedImage();

			String mainKey = makeKey(id,
					SpecailKeyWindowImageType.SPECIAL_IMAGE_TYPE_NORMAL);
			keyWindowImageMap.put(mainKey,
					new Image(display, imagePath + File.separator + mainImage));

			String pressedKey = makeKey(id,
					SpecailKeyWindowImageType.SPECIAL_IMAGE_TYPE_PRESSED);
			keyWindowImageMap.put(pressedKey,
					new Image(display, imagePath + File.separator + keyPressedImage));

			image = keyWindowImageMap.get(makeKey(id, imageType));
		}

		return image;
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
