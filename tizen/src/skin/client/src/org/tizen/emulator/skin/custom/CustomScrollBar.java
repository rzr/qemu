/**
 *
 *
 * Copyright ( C ) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or ( at your option ) any later version.
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
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.tizen.emulator.skin.log.SkinLogger;


public class CustomScrollBar {
	static final int SCROLL_SHIFT_LENGTH = 10;

	private Logger logger = SkinLogger.getSkinLogger(
			CustomScrollBar.class).getLogger();

	private Composite parent;
	private Composite composite;
	CustomScrolledComposite compositeScroll;

	private int heightScrollBar;
	private int maxShift;

	private Image[] imagesArrowUp;
	private Image[] imagesArrowDown;
	private Image imageThumb;
	private Image imageShaft;

	private CustomButton buttonArrowUp;
	private CustomButton buttonArrowDown;
	private Canvas canvasShaft;
	private CustomButton buttonThumb;

	private int valueSelection;
	private Timer timerScroller;
	private TimerTask scroller;

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
		this.maxShift = SCROLL_SHIFT_LENGTH;

		this.imagesArrowUp = imagesArrowUp;
		this.imagesArrowDown = imagesArrowDown;
		this.imageThumb = imageThumb;
		this.imageShaft = imageShaft;

		//this.timerScroller = new Timer();

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

		final int widthShaft = width;
		final int heightShaft = heightScrollBar - (height * 2);
		canvasShaft.setLayoutData(new RowData(widthShaft, heightShaft));

		canvasShaft.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				if (imageShaft != null) {
					e.gc.drawImage(imageShaft, 0, 0,
							imageShaft.getImageData().width,
							imageShaft.getImageData().height,
							0, 0, widthShaft, heightShaft);
				}
			}
		});

		/* arrow down */
		buttonArrowDown = new CustomButton(composite, SWT.NONE,
				imagesArrowDown[0], imagesArrowDown[1], imagesArrowDown[2]);

		buttonArrowDown.setBackground(parent.getBackground());
		buttonArrowDown.setLayoutData(new RowData(width, height));
	}

	class ScrollerTask extends TimerTask {
		@Override
		public void run() {
			int vSelection = getSelection();
			if (vSelection <= (173 - heightScrollBar - 1)) {
				setSelection(getSelection() + 1);
				logger.info("" + getSelection());

//				Display.getCurrent().asyncExec(new Runnable() {
//					@Override
//					public void run() {
				//compositeScroll.vScroll();
//					}
//				});
			}
		}
	}

	protected void addScrollBarListener() {
		buttonArrowUp.addMouseListener(new MouseListener() {
			@Override
			public void mouseDown(MouseEvent e) {
				int shift = getSelection();

				if (shift > 0) {
					setSelection(getSelection() - Math.min(maxShift, shift));
					((CustomScrolledComposite) parent.getParent()).vScroll();
				}
			}

			@Override
			public void mouseUp(MouseEvent e) {
				/* do nothing */
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		});

		buttonArrowDown.addMouseListener(new MouseListener() {
			@Override
			public void mouseDown(MouseEvent e) {
				int minHeightContents =
						((CustomScrolledComposite) parent.getParent()).getMinHeight();

				int shift = (minHeightContents - heightScrollBar) - getSelection();

				if (shift > 0) {
					setSelection(getSelection() + Math.min(maxShift, shift));
					((CustomScrolledComposite) parent.getParent()).vScroll();
				}
			}

			@Override
			public void mouseUp(MouseEvent e) {
				timerScroller.cancel();
				timerScroller = new Timer();
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		});

//		buttonArrowDown.addDragDetectListener(new DragDetectListener() {
//			@Override
//			public void dragDetected( DragDetectEvent e ) {
//					logger.info( "dragDetected:" + e.button );
//					timerScroller.schedule(new ScrollerTask(), 1, 100);
//			}
//		});
	}

	public int getSelection() {
		return valueSelection;
	}

	public void setSelection(int selection) {
		valueSelection = selection;
	}
}
