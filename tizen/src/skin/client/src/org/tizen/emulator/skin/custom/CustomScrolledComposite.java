/**
 * Custom Scrolled Composite
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
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Listener;
import org.tizen.emulator.skin.log.SkinLogger;

public class CustomScrolledComposite extends Composite {

	private Logger logger =
			SkinLogger.getSkinLogger(CustomScrolledComposite.class).getLogger();

	Control content;
	Listener contentListener;
	Listener filter;
	Point sizeContent;

	Composite compositeRight;
	CustomScrollBar vBar;

	private Image[] imagesArrowUp;
	private Image[] imagesArrowDown;
	private Image imageThumb;
	private Image imageShaft;

	public int minHeight = 0;
	public int minWidth = 0;
	public boolean expandHorizontal = false;
	public boolean expandVertical = false;

	/**
	 *  Constructor
	 */
	public CustomScrolledComposite(Composite parent, int style,
			Image[] imagesArrowUp, Image[] imagesArrowDown,
			Image imageThumb, Image imageShaft) {
		super(parent, style);
		super.setLayout(new ScrolledCompositeLayout());

		compositeRight = new Composite(this, SWT.NONE);
		compositeRight.setBackground(Display.getDefault().getSystemColor(SWT.COLOR_YELLOW));

		GridLayout compositeGridLayout = new GridLayout(1, false);
		compositeGridLayout.marginLeft = compositeGridLayout.marginRight = 0;
		compositeGridLayout.marginTop = compositeGridLayout.marginBottom = 0;
		compositeGridLayout.marginWidth = compositeGridLayout.marginHeight = 0;
		compositeGridLayout.horizontalSpacing = compositeGridLayout.verticalSpacing = 0;
		compositeRight.setLayout(compositeGridLayout);

		this.imagesArrowUp = imagesArrowUp;
		this.imagesArrowDown = imagesArrowDown;
		this.imageThumb = imageThumb;
		this.imageShaft = imageShaft;
	}

	public CustomScrollBar getScrollBar() {
		return vBar;
	}

	public void setContent(Control content, Point sizeContent) {
		checkWidget();
		if (this.content != null && !this.content.isDisposed()) {
			this.content.removeListener(SWT.Resize, contentListener);
			this.content.setBounds(new Rectangle(-200, -200, 0, 0));
		}

		this.content = content;
		this.sizeContent = sizeContent;

		if (this.content != null) {
			if (vBar == null) {
				compositeRight.setBackground(this.content.getBackground());
				vBar = new CustomScrollBar(compositeRight, SWT.NONE, sizeContent.y,
						imagesArrowUp, imagesArrowDown, imageThumb, imageShaft);
			}

			content.setLocation(0, 0);

			layout(false);

		}
	}

	public void setExpandHorizontal(boolean expand) {
		checkWidget();
		if (expand == expandHorizontal) return;
		expandHorizontal = expand;
		layout(false);
	}

	public void setExpandVertical(boolean expand) {
		checkWidget();
		if (expand == expandVertical) return;
		expandVertical = expand;
		layout(false);
	}

	public void setMinSize(Point size) {
		if (size == null) {
			setMinSize(0, 0);
		} else {
			setMinSize(size.x, size.y);
		}
	}

	public void setMinSize(int width, int height) {
		checkWidget();
		if (width == minWidth && height == minHeight) {
			return;
		}

		minWidth = Math.max(0, width);
		minHeight = Math.max(0, height);

		logger.info("composite minWidth : " + minWidth +
				", minHeight : " + minHeight);

		layout(false);
	}

	public int getMinWidth() {
		checkWidget();
		return minWidth;
	}

	public int getMinHeight() {
		checkWidget();
		return minHeight;
	}

	public void vScroll() {
		if (content == null) {
			return;
		}

		Point location = content.getLocation();
		int vSelection = vBar.getSelection();

		content.setLocation(location.x, -vSelection);
	}
}