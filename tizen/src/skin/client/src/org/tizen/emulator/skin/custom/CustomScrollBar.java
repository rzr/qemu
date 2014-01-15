/**
 * Custom Scroll Bar
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

import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DragDetectEvent;
import org.eclipse.swt.events.DragDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseWheelListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.tizen.emulator.skin.log.SkinLogger;


class CustomScrollBarThumbData {
	public boolean isGrabbed;
	public int yGrabPosition;
	public int yGrabSelection;

	public Rectangle boundsThumb;

	CustomScrollBarThumbData() {
		boundsThumb = new Rectangle(0, 0, 0, 0);
	}
}

class CustomScrollBarShaftData {
	public int widthShaft;
	public int heightShaft;
}

public class CustomScrollBar {
	static final int SCROLL_INCREMENT_AMOUNT = 10;
	static final int SCROLL_PAGE_INCREMENT_AMOUNT = 144;

	private Logger logger = SkinLogger.getSkinLogger(
			CustomScrollBar.class).getLogger();

	private Composite parent;
	private Composite composite;
	CustomScrolledComposite compositeScroll;

	private int heightScrollBar;
	private int amountIncrement;
	private int amountPageIncrement;

	private Image[] imagesArrowUp;
	private Image[] imagesArrowDown;
	private Image imageThumb;
	private Image imageShaft;

	private CustomButton buttonArrowUp;
	private CustomButton buttonArrowDown;
	private Canvas canvasShaft;
	private CustomScrollBarThumbData dataThumb;
	private CustomScrollBarShaftData dataShaft;

	private int valueSelection;
	private Timer timerScroller;

	class ScrollerTask extends TimerTask {
		static final int SCROLLER_PERIOD_TIME = 60;

		private boolean isScrollDown;

		ScrollerTask(boolean isDown) {
			this.isScrollDown = isDown;
		}

		@Override
		public void run() {
			Display.getDefault().asyncExec(new Runnable() {
				@Override
				public void run() {
					if (isScrollDown == true) {
						scrollDown(amountIncrement * 2);
					} else {
						scrollUp(amountIncrement * 2);
					}
				}
			});
		}
	}

	/**
	 *  Constructor
	 */
	CustomScrollBar(Composite parent, int style, int heightScrollBar,
			Image[] imagesArrowUp, Image[] imagesArrowDown,
			Image imageThumb, Image imageShaft) {
		this.parent = parent;
		this.composite = new Composite(parent, SWT.NONE);
		this.compositeScroll = ((CustomScrolledComposite) parent.getParent());

		RowLayout rowLayout = new RowLayout(SWT.VERTICAL);
		rowLayout.marginLeft = rowLayout.marginRight = 0;
		rowLayout.marginTop = rowLayout.marginBottom = 0;
		rowLayout.marginWidth = rowLayout.marginHeight = 0;
		rowLayout.spacing = 0;

		composite.setLayout(rowLayout);

		this.heightScrollBar = heightScrollBar;
		this.amountIncrement = SCROLL_INCREMENT_AMOUNT;
		this.amountPageIncrement = SCROLL_PAGE_INCREMENT_AMOUNT;

		this.imagesArrowUp = imagesArrowUp;
		this.imagesArrowDown = imagesArrowDown;
		this.imageThumb = imageThumb;
		this.imageShaft = imageShaft;

		this.dataThumb = new CustomScrollBarThumbData();
		this.dataShaft = new CustomScrollBarShaftData();

		this.timerScroller = null;
		createContents();

		addScrollBarListener();
	}

	protected void createContents() {
		/* arrow up */
		buttonArrowUp = new CustomButton(composite, SWT.NONE,
				imagesArrowUp[0], imagesArrowUp[1], imagesArrowUp[2]);

		int width = buttonArrowUp.getImageSize().x;
		int height = buttonArrowUp.getImageSize().y;

		buttonArrowUp.setBackground(parent.getBackground());
		buttonArrowUp.setLayoutData(new RowData(width, height));

		/* shaft */
		canvasShaft = new Canvas(composite, SWT.NONE);
		canvasShaft.setBackground(parent.getBackground());

		dataShaft.widthShaft = width;
		dataShaft.heightShaft = heightScrollBar - (height * 2);
		canvasShaft.setLayoutData(new RowData(
				dataShaft.widthShaft, dataShaft.heightShaft));

		canvasShaft.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				if (imageShaft != null) {
					e.gc.drawImage(imageShaft, 0, 0,
							imageShaft.getImageData().width,
							imageShaft.getImageData().height,
							0, 0, dataShaft.widthShaft, dataShaft.heightShaft);

					/* draw a thumb */
					int heightScrollGap = compositeScroll.getMinHeight() - heightScrollBar;

					int tempHeightThumb = (compositeScroll.getMinHeight() - heightScrollGap) *
							dataShaft.heightShaft / compositeScroll.getMinHeight();
					dataThumb.boundsThumb.height = Math.max(1, tempHeightThumb);

					dataThumb.boundsThumb.x = 2;
					dataThumb.boundsThumb.y = getSelection() *
							(dataShaft.heightShaft - dataThumb.boundsThumb.height) /
							heightScrollGap;
					dataThumb.boundsThumb.width = dataShaft.widthShaft -
							(dataThumb.boundsThumb.x * 2);

					e.gc.drawImage(imageThumb, 0, 0,
							imageThumb.getImageData().width,
							imageThumb.getImageData().height,
							dataThumb.boundsThumb.x, dataThumb.boundsThumb.y,
							dataThumb.boundsThumb.width, dataThumb.boundsThumb.height);
				}
			}
		});

		/* arrow down */
		buttonArrowDown = new CustomButton(composite, SWT.NONE,
				imagesArrowDown[0], imagesArrowDown[1], imagesArrowDown[2]);

		buttonArrowDown.setBackground(parent.getBackground());
		buttonArrowDown.setLayoutData(new RowData(width, height));
	}

	private void updateScrollbar() {
		compositeScroll.vScroll();
		canvasShaft.redraw();
	}

	private void scrollUp(int amount) {
		if (amount == 0) {
			return;
		}

		setSelection(getSelection() - amount);
		updateScrollbar();
	}

	private void scrollDown(int amount) {
		if (amount == 0) {
			return;
		}

		setSelection(getSelection() + amount);
		updateScrollbar();
	}

	protected void addScrollBarListener() {
		buttonArrowUp.addMouseListener(new MouseListener() {
			@Override
			public void mouseDown(MouseEvent e) {
				scrollUp(amountIncrement);
			}

			@Override
			public void mouseUp(MouseEvent e) {
				if (timerScroller != null) {
					timerScroller.cancel();
					timerScroller = null;
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		});

		buttonArrowUp.addDragDetectListener(new DragDetectListener() {
			@Override
			public void dragDetected(DragDetectEvent e) {
				logger.info("ArrowUp dragDetected : " + e.button);

				if (timerScroller != null) {
					timerScroller.cancel();
				}

				timerScroller = new Timer();
				timerScroller.schedule(new ScrollerTask(false),
						1, ScrollerTask.SCROLLER_PERIOD_TIME);
			}
		});

		buttonArrowDown.addMouseListener(new MouseListener() {
			@Override
			public void mouseDown(MouseEvent e) {
				scrollDown(amountIncrement);
			}

			@Override
			public void mouseUp(MouseEvent e) {
				if (timerScroller != null) {
					timerScroller.cancel();
					timerScroller = null;
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		});

		buttonArrowDown.addDragDetectListener(new DragDetectListener() {
			@Override
			public void dragDetected(DragDetectEvent e) {
				logger.info("ArrowDown dragDetected : " + e.button);

				if (timerScroller != null) {
					timerScroller.cancel();
				}

				timerScroller = new Timer();
				timerScroller.schedule(new ScrollerTask(true),
						1, ScrollerTask.SCROLLER_PERIOD_TIME);
			}
		});

		compositeScroll.addMouseWheelListener(new MouseWheelListener() {
			@Override
			public void mouseScrolled(MouseEvent e) {
				if (e.count > 0) {
					scrollUp(amountIncrement);
				} else {
					scrollDown(amountIncrement);
				}
			}
		});

		canvasShaft.addMouseMoveListener(new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (dataThumb.isGrabbed == true) {
					int yDragged = e.y - dataThumb.yGrabPosition;

					int yDraggedScale = yDragged *
							(compositeScroll.getMinHeight() - heightScrollBar) /
							(dataShaft.heightShaft - dataThumb.boundsThumb.height);

					setSelection(dataThumb.yGrabSelection + yDraggedScale);
					updateScrollbar();
				}
			}
		});

		canvasShaft.addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}

			@Override
			public void mouseDown(MouseEvent e) {
				Rectangle rectThumb = new Rectangle(
						0, dataThumb.boundsThumb.y,
						dataThumb.boundsThumb.width + 2, dataThumb.boundsThumb.height);

				if (rectThumb.contains(e.x, e.y) == true) {
					dataThumb.isGrabbed = true;
					dataThumb.yGrabPosition = e.y;
					dataThumb.yGrabSelection = getSelection();
				} else {
					if (e.y < dataThumb.boundsThumb.y) {
						scrollUp(amountPageIncrement);
					} else {
						scrollDown(amountPageIncrement);
					}
				}
			}

			@Override
			public void mouseUp(MouseEvent e) {
				if (dataThumb.isGrabbed == true) {
					dataThumb.isGrabbed = false;
					dataThumb.yGrabSelection = dataThumb.yGrabPosition = 0;
				}
			}
		});
	}

	public int getSelection() {
		return valueSelection;
	}

	public void setSelection(int selection) {
		valueSelection = selection;

		if (valueSelection < 0) {
			valueSelection = 0;
		} else {
			int maxScroll = compositeScroll.getMinHeight() - heightScrollBar;
			valueSelection = Math.min(selection, maxScroll);
		}
	}
}
