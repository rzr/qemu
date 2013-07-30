/**
 * child window of skin
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

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.log.SkinLogger;

public class SkinWindow {
	protected Logger logger = SkinLogger.getSkinLogger(
			SkinWindow.class).getLogger();

	protected Shell shell;
	protected Shell parent;
	private int dockPosition;

	public SkinWindow(Shell parent, int dockPosition) {
		this.parent = parent;
		this.dockPosition = dockPosition;
	}

	public Shell getShell() {
		return shell;
	}

	public void open(int dockValue) {
		if (shell.isDisposed()) {
			return;
		}

		dock(dockValue, true, true);

		shell.open();
	}

	public void dock(int dockValue,
			boolean correction, boolean enableLogger) {
		int x = 0;
		int y = 0;

		Rectangle monitorBounds = Display.getDefault().getBounds();
		Rectangle parentBounds = parent.getBounds();
		Rectangle childBounds = shell.getBounds();

		if (enableLogger == true) {
			logger.info("host monitor display bounds : " + monitorBounds);
			logger.info("current parent shell bounds : " + parentBounds);
			logger.info("current child shell bounds : " + childBounds);
		}

		dockPosition = dockValue;

		if (dockPosition == SWT.NONE){
			logger.info("undock");
			/* do nothing */

			return;
		} else if (dockPosition == (SWT.RIGHT | SWT.TOP)) {
			x = parentBounds.x + parentBounds.width;
			y = parentBounds.y;

			/* correction of location */
			/*if ((x + childBounds.width) >
					(monitorBounds.x + monitorBounds.width)) {
				x = parentBounds.x - childBounds.width;
			}*/
		} else if (dockPosition == (SWT.RIGHT | SWT.BOTTOM)) {
			x = parentBounds.x + parentBounds.width;
			y = parentBounds.y + parentBounds.height - childBounds.height;

			/* correction of location */
			/*int shift = (monitorBounds.x + monitorBounds.width) -
					(x + childBounds.width);
			if (shift < 0) {
				x += shift;
				parent.setLocation(parentBounds.x + shift, parentBounds.y);
			}*/
		}
		else if (dockPosition == (SWT.LEFT | SWT.CENTER)) {
			x = parentBounds.x - childBounds.width;
			y = parentBounds.y + (parentBounds.height / 2) -
					(childBounds.height / 2);
		} else if (dockPosition == (SWT.LEFT | SWT.TOP)) {
			x = parentBounds.x - childBounds.width;
			y = parentBounds.y;
		} else if (dockPosition == (SWT.LEFT | SWT.BOTTOM)) {
			x = parentBounds.x - childBounds.width;
			y = parentBounds.y + parentBounds.height - childBounds.height;
		}
		else { /* SWT.RIGHT | SWT.CENTER */
			x = parentBounds.x + parentBounds.width;
			y = parentBounds.y + (parentBounds.height / 2) -
					(childBounds.height / 2);
		}

		/* correction of location */
		if (correction == true) {
			/* for right side */
			int shift = (monitorBounds.x + monitorBounds.width) -
					(x + childBounds.width);
			if (shift < 0) {
				x += shift;
				parent.setLocation(parentBounds.x + shift, parentBounds.y);
			}

			/* for left side */
			shift = monitorBounds.x - x;
			if (shift > 0) {
				x += shift;
				parent.setLocation(parentBounds.x + shift, parentBounds.y);
			}
		}

		shell.setLocation(x, y);
	}

	public int getDockPosition() {
		return dockPosition;
	}
}
