/**
 * Pair Tag
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

package org.tizen.emulator.skin.custom;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Shell;

public class ColorTag extends Canvas {
	private static final int COLORTAG_WIDTH = 8;
	private static final int COLORTAG_HEIGHT = 8;

	public ColorTag(final Shell parent, int style, final Color colorOval) {
		super(parent, style);

		setSize(COLORTAG_WIDTH, COLORTAG_HEIGHT);
		setBackground(parent.getBackground());

		addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				e.gc.setBackground(colorOval);
				e.gc.setAntialias(SWT.ON);
				e.gc.fillOval(
						0, 0, COLORTAG_WIDTH, COLORTAG_HEIGHT);
			}
		});
	}

	public int getWidth() {
		return getSize().x;
	}

	public int getHeight() {
		return getSize().y;
	}
}