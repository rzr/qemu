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
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.log.SkinLogger;

public class CustomProgressBar extends Canvas {
	public static final RGB PROGRESS_COLOR_0 = new RGB(0, 174, 239);
	public static final RGB PROGRESS_COLOR_1 = new RGB(179, 246, 0);

	private Logger logger =
			SkinLogger.getSkinLogger(CustomProgressBar.class).getLogger();

	private EmulatorSkin skin;
	private Composite parent;
	private boolean isDual;
	private int[] selection;
	private Color[] colorProgress;

	/**
	 *  Constructor
	 */
	public CustomProgressBar(EmulatorSkin skin, int style, boolean isDual) {
		super(skin.getShell(), style);

		this.skin = skin;
		this.parent = skin.getShell();
		this.isDual = isDual;
		this.selection = new int[2];
		this.selection[0] = this.selection[1] = 0;

		this.colorProgress = new Color[2];
		this.colorProgress[0] = new Color(parent.getDisplay(), PROGRESS_COLOR_0);
		if (isDual == true) {
			logger.info("dual progress bar is created");
			this.colorProgress[1] = new Color(parent.getDisplay(), PROGRESS_COLOR_1);
		} else {
			logger.info("single progress bar is created");
		}

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
				if (isDual == true) {
					/* draw layer1 */
					e.gc.setBackground(colorProgress[1]);
					Rectangle bounds = getBounds();
					int width = (bounds.width * selection[1]) / 100;
					e.gc.fillRectangle(0, 0,
							Math.min(bounds.width, width), bounds.height);
				}

				/* draw layer0 */
				e.gc.setBackground(colorProgress[0]);
				Rectangle bounds = getBounds();
				int width = (bounds.width * selection[0]) / 100;
				e.gc.fillRectangle(0, 0,
						Math.min(bounds.width, width), bounds.height);

				if (selection[0] == 101 &&
						(isDual == false || selection[1] == 101)) {
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

				colorProgress[0].dispose();
				if (isDual == true) {
					colorProgress[1].dispose();
				}

				skin.bootingProgress = null;
			}
		});
	}

	public void setSelection(int idxLayer, int value) {
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

		if (idxLayer < 0 || isDual == false) {
			idxLayer = 0;
		} else if (idxLayer > 1) {
			idxLayer = 1;
		}

		if (value < 0) {
			value = 0;
		} else if (value > 100) {
			value = 100;
		}

		selection[idxLayer] = value;
		logger.info("layer" + idxLayer + " progress : " + selection[idxLayer]);

		final int index = idxLayer;
		parent.getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				redraw();

				if (selection[index] == 100) {
					selection[index] = 101;
				}
			}
		});
	}

	public int getSelection(int idxLayer) {
		return selection[idxLayer];
	}
}
