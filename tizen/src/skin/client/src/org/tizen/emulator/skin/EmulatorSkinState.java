/**
 * Emulator Skin Process
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

package org.tizen.emulator.skin;

import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;

public class EmulatorSkinState {
	private Point currentResolution;
	private int currentScale;
	private short currentRotationId;
	private int currentAngle;

	private Image currentImage;
	private Image currentKeyPressedImage;

	public EmulatorSkinState() {
		this.currentResolution = new Point(480, 800);
		this.currentScale = 50;
		this.currentRotationId = RotationInfo.PORTRAIT.id();
		this.currentAngle = 0;
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

	public synchronized void setCurrentRotationId(short rotationId) {
		this.currentRotationId = rotationId;
	}

	/* angle */
	public synchronized int getCurrentAngle() {
		return currentAngle;
	}

	public synchronized void setCurrentAngle(int angle) {
		this.currentAngle = angle;
	}

	/* skin image */
	public Image getCurrentImage() {
		return currentImage;
	}

	public void setCurrentImage(Image image) {
		this.currentImage = image;
	}

	public Image getCurrentKeyPressedImage() {
		return currentKeyPressedImage;
	}

	public void setCurrentKeyPressedImag(Image keyPressedImage) {
		this.currentKeyPressedImage = keyPressedImage;
	}
}
