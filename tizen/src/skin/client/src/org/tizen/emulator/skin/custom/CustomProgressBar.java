/**
 * Custom Progress Bar
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

package org.tizen.emulator.skin.custom;

import java.util.logging.Logger;

import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.tizen.emulator.skin.log.SkinLogger;

public class CustomProgressBar extends Canvas {
	public static final RGB PROGRESS_COLOR = new RGB(0, 173, 239);

	private Logger logger =
			SkinLogger.getSkinLogger(CustomProgressBar.class).getLogger();

	private Composite parent;
	private int selection;
	private Color colorProgress;

	/**
	 *  Constructor
	 */
	public CustomProgressBar(final Composite parent, int style) {
		super(parent, style);

		this.parent = parent;
		this.selection = 0;
		this.colorProgress = new Color(parent.getDisplay(), PROGRESS_COLOR);

		addProgressBarListener();

		/* default is hidden */
		parent.getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				setVisible(false);
			}
		});
	}

	protected void addProgressBarListener() {
		addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				e.gc.setBackground(colorProgress);

				Rectangle bounds = getBounds();
				int width = (bounds.width * selection) / 100; 
				e.gc.fillRectangle(0, 0, width, bounds.height);

				if (selection == -1) {
					logger.info("progress : complete!");

					parent.getDisplay().asyncExec(new Runnable() {
						@Override
						public void run() {
							dispose();
						}
					});
				}
			}
		});

		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e) {
				logger.info("progress bar is disposed");

				colorProgress.dispose();
			}
		});
	}

	public void setSelection(int value) {
		if (isDisposed() == true) {
			return;
		}

		if (isVisible() == false) {
			parent.getDisplay().syncExec(new Runnable() {
				@Override
				public void run() {
					setVisible(true);
				}
			});
		}

		if (value < 0) {
			value = 0;
		} else if (value > 100) {
			value = 100;
		}

		selection = value;
		logger.info("progress : " + selection);

		parent.getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				redraw();

				if (selection == 100) {
					selection = -1;
				}
			}
		});
	}

	public int getSelection() {
		return selection;
	}
}
