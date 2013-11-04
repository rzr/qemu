/**
 * Custom Scrolled Composite Layout
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

import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Layout;
import org.tizen.emulator.skin.log.SkinLogger;

class ScrolledCompositeLayout extends Layout {
	static final int DEFAULT_WIDTH = 64;
	static final int DEFAULT_HEIGHT = 64;

	private Logger logger = SkinLogger.getSkinLogger(
			ScrolledCompositeLayout.class).getLogger();

	private boolean inLayout = false;

	protected Point computeSize(Composite composite, int wHint, int hHint, boolean flushCache) {
		CustomScrolledComposite sc = (CustomScrolledComposite)composite;
		Point size = new Point(DEFAULT_WIDTH, DEFAULT_HEIGHT);

		if (sc.content != null) {
			Point preferredSize = sc.content.computeSize(wHint, hHint, flushCache);
			size.x = preferredSize.x;
			size.y = preferredSize.y;
		}
		size.x = Math.max(size.x, sc.minWidth);
		size.y = Math.max(size.y, sc.minHeight);

		if (wHint != SWT.DEFAULT) size.x = wHint;
		if (hHint != SWT.DEFAULT) size.y = hHint;

		return size;
	}

	protected boolean flushCache(Control control) {
		return true;
	}

	protected void layout(Composite composite, boolean flushCache) {
		if (inLayout) {
			return;
		}

		logger.info("layouting");

		CustomScrolledComposite sc = (CustomScrolledComposite)composite;
		if (sc.content == null) {
			return;
		}

		inLayout = true;
		Rectangle contentRect = sc.content.getBounds();

		Rectangle hostRect = sc.getClientArea();
		if (sc.expandHorizontal) {
			contentRect.width = Math.max(sc.minWidth, hostRect.width);
		}
		if (sc.expandVertical) {
			contentRect.height = Math.max(sc.minHeight, hostRect.height);
		}

		sc.content.setBounds(contentRect);

		Point size = sc.compositeRight.computeSize(SWT.DEFAULT, SWT.DEFAULT);
		sc.compositeRight.setBounds(
				contentRect.width - size.x, 0, size.x, contentRect.height);

		inLayout = false;
	}
}
