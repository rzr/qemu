/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Hyunjun Son <hj79.son@samsung.com>
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

package org.tizen.emulator.skin.dialog;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Dialog;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.util.StringUtil;

/**
 * 
 *
 */
public abstract class SkinDialog extends Dialog {

	protected Shell shell;
	private boolean isReady;
	private Composite buttonComposite;
	private Shell parent;
	private String title;
	private int style;
	private boolean pack;

	public SkinDialog( Shell parent, String title, int style ) {
		super( parent, style );
		this.parent = parent;
		this.title = title;
		this.style = style;
		this.pack = true;
	}

	public SkinDialog( Shell parent, String title, int style, boolean pack ) {
		this( parent, title, style );
		this.pack = pack;
	}

	public void open() {

		shell = new Shell( parent, style );
		shell.setLocation( parent.getLocation().x + 30, parent.getLocation().y + 100 );
		shell.setText( title );
		shell.setLayout( new GridLayout( 1, true ) );

		Composite parent = new Composite( shell, SWT.NONE );
		GridLayout gridLayout = new GridLayout( 1, true );
		gridLayout.marginWidth = 20;
		parent.setLayout( gridLayout );

		Composite composite = new Composite( parent, SWT.NONE );
		composite.setLayoutData( new GridData( GridData.FILL_HORIZONTAL ) );
		composite.setLayout( new GridLayout( 1, true ) );

		Composite area = createArea( composite );
		if ( null == area ) {
			return;
		}

		isReady = true;

		buttonComposite = new Composite( parent, SWT.NONE );
		buttonComposite.setLayoutData( new GridData( GridData.FILL_HORIZONTAL ) );
		buttonComposite.setLayout( new FillLayout( SWT.HORIZONTAL ) );

		createButtons( buttonComposite );

		if ( pack ) {
			shell.pack();
		}

		if ( !isReady ) {
			return;
		}

		shell.open();

		while ( !shell.isDisposed() ) {
			if ( !shell.getDisplay().readAndDispatch() ) {
				shell.getDisplay().sleep();
			}
		}

	}

	protected abstract Composite createArea( Composite parent );

	protected void createButtons( Composite parent ) {
		if ( null == parent ) {
			throw new IllegalArgumentException( "Buttons parent is null" );
		}
	}

	protected Button createButton( Composite parent, String text ) {

		if ( null == parent ) {
			throw new IllegalArgumentException( "Button parent is null" );
		}

		Composite composite = new Composite( parent, SWT.NONE );
		GridLayout gridLayout = new GridLayout( 1, true );
		composite.setLayout( gridLayout );

		Button button = new Button( composite, SWT.PUSH );
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;

		button.setLayoutData( gd );
		button.setText( StringUtil.nvl( text ) );
		return button;

	}

	protected Button createCloseButton( Composite parent, boolean setFocus ) {

		Button closeButton = createButton( parent, "Close" );
		closeButton.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				SkinDialog.this.shell.close();
			}
		} );

		if ( setFocus ) {
			closeButton.setFocus();
		}

		return closeButton;

	}

}
