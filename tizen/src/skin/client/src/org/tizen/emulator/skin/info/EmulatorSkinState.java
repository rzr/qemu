/**
 * Emulator Skin State
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin.info;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.layout.rotation.SkinRotations;

public class EmulatorSkinState {
	private Point currentResolution;
	private int currentScale;
	private short currentRotationId;

	private Rectangle displayBounds;
	private boolean updateDisplayBounds;

	private Image currentImage;
	private Image currentKeyPressedImage;
	private Color hoverColor;

	/**
	 *  Constructor
	 */
	public EmulatorSkinState() {
		this.currentResolution = new Point(720, 1280);
		this.currentScale = EmulatorConfig.DEFAULT_WINDOW_SCALE;
		this.currentRotationId = SkinRotations.PORTRAIT_ID;

		this.displayBounds = null;
		this.updateDisplayBounds = false;
	}

	/* resolution */
	public synchronized Point getCurrentResolution() {
		return currentResolution;
	}

	public synchronized int getCurrentResolutionWidth() {
		return currentResolution.x;
	}

	public synchronized int getCurrentResolutionHeight() {
		return currentResolution.y;
	}

	public synchronized void setCurrentResolution(Point resolution) {
		setCurrentResolutionWidth(resolution.x);
		setCurrentResolutionHeight(resolution.y);
	}

	public synchronized void setCurrentResolutionWidth(int width) {
		if (width < 0) {
			width = 0;
		}
		this.currentResolution.x = width;
	}

	public synchronized void setCurrentResolutionHeight(int height) {
		if (height < 0) {
			height = 0;
		}
		this.currentResolution.y = height;
	}

	/* scale */
	public synchronized int getCurrentScale() {
		return currentScale;
	}

	public synchronized void setCurrentScale(int scale) {
		this.currentScale = scale;
	}

	/* rotation */
	public synchronized short getCurrentRotationId() {
		return currentRotationId;
	}

	public synchronized void setCurrentRotationId() {
		this.currentRotationId = SkinRotations.PORTRAIT_ID;
	}

	public synchronized void setCurrentRotationId(short rotationId) {
		this.currentRotationId = rotationId;
	}

	/* display bounds */
	public synchronized Rectangle getDisplayBounds() {
		if (displayBounds == null) {
			return new Rectangle(0, 0, 10, 10);
		}

		return displayBounds;
	}

	public synchronized void setDisplayBounds(Rectangle bounds) {
		this.displayBounds = bounds;
	}

	public synchronized boolean isNeedToUpdateDisplay() {
		return updateDisplayBounds;
	}

	public synchronized void setNeedToUpdateDisplay(boolean needUpdate) {
		this.updateDisplayBounds = needUpdate;
	}

	/* skin image */
	public synchronized Image getCurrentImage() {
		return currentImage;
	}

	public synchronized void setCurrentImage(Image image) {
		this.currentImage = image;
	}

	public synchronized Image getCurrentKeyPressedImage() {
		return currentKeyPressedImage;
	}

	public synchronized void setCurrentKeyPressedImage(Image keyPressedImage) {
		this.currentKeyPressedImage = keyPressedImage;
	}

	/* color of hover */
	public synchronized Color getHoverColor() {
		return hoverColor;
	}

	public synchronized void setHoverColor(Color color) {
		this.hoverColor = color;
	}
}
