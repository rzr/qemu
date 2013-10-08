/**
 * Image Resource Management For Profile-Specific Skin
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
import org.tizen.emulator.skin.util.SkinRotation;

public class ProfileSkinImageRegistry {
	private static Logger logger = SkinLogger.getSkinLogger(
			ProfileSkinImageRegistry.class).getLogger();

	public enum SkinImageType {
		PROFILE_IMAGE_TYPE_NORMAL,
		PROFILE_IMAGE_TYPE_PRESSED
	}

	private Display display;
	private String skinPath;
	private EmulatorUI dbiContents;
	private Map<String, Image> skinImageMap;

	/**
	 *  Constructor
	 */
	public ProfileSkinImageRegistry(EmulatorConfig config,
			Display display, String skinPath) {
		this.display = display;
		this.skinPath = skinPath;
		this.dbiContents = config.getDbiContents();
		this.skinImageMap = new HashMap<String, Image>();

		initialize(skinPath);
	}

	private void initialize(String argSkinPath) {
		RotationsType rotations = dbiContents.getRotations();

		if (null == rotations) {
			logger.severe("Fail to loading rotations element from XML");
			return;
		}

		List<RotationType> rotationList = rotations.getRotation();

		for (RotationType rotation : rotationList) {
			SkinRotation.put(rotation);
		}
	}

	private String makeKey(Short id, SkinImageType imageType) {
		return id + ":" + imageType.ordinal();
	}

	public Image getSkinImage(Short id, SkinImageType imageType) {
		Image image = skinImageMap.get(makeKey(id, imageType));

		if (image == null) {
			RotationsType rotations = dbiContents.getRotations();

			if (null == rotations) {
				logger.severe("Fail to loading rotations element from XML");
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
					String mainKey = makeKey(id, SkinImageType.PROFILE_IMAGE_TYPE_NORMAL);
					skinImageMap.put(mainKey,
							new Image(display, skinPath + File.separator + mainImage));

					String pressedKey = makeKey(id, SkinImageType.PROFILE_IMAGE_TYPE_PRESSED);
					skinImageMap.put(pressedKey,
							new Image(display, skinPath + File.separator + keyPressedImage));

					break;
				}
			}

			image = skinImageMap.get(makeKey(id, imageType));
		}

		return image;
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
