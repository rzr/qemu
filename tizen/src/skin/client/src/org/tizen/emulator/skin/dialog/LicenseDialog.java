/**
 * 
 *
 * Copyright (C) 2011 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * HyunJun Son <hj79.son@samsung.com>
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

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.StringUtil;

/**
 * 
 *
 */
public class LicenseDialog extends SkinDialog {

	public static final String LICENSE_FILE_PATH = "../license/Open_Source_Announcement.txt";

	private Logger logger = SkinLogger.getSkinLogger( LicenseDialog.class ).getLogger();

	public LicenseDialog( Shell parent, String title ) {
		super( parent, title, SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL | SWT.RESIZE | SWT.MAX );
	}

	@Override
	protected Composite createArea( Composite parent ) {

		Composite composite = new Composite( parent, SWT.NONE );
		composite.setLayout( new GridLayout() );

		final Text text = new Text( composite, SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL | SWT.MULTI );
		GridData gridData = new GridData( SWT.FILL, SWT.FILL, true, true );
		text.setLayoutData( gridData );

		text.setEditable( false );
		text.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WHITE ) );
		String license = StringUtil.nvl( getLicense() );
		text.setText( license );

		return composite;

	}

	@Override
	protected void setShellSize() {
		shell.setSize( (int) ( 400 * 1.618 ), 400 );
	}

	@Override
	protected void createButtons( Composite parent ) {
		super.createButtons( parent );

		Button okButton = createButton( parent, OK );

		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.RIGHT;
		okButton.setLayoutData( gd );

		okButton.setFocus();

		okButton.addSelectionListener( new SelectionAdapter() {
			@Override
			public void widgetSelected( SelectionEvent e ) {
				LicenseDialog.this.shell.close();
			}
		} );

	}

	private String getLicense() {

		FileInputStream fis = null;
		String string = "";

		try {

			fis = new FileInputStream( LICENSE_FILE_PATH );

			try {
				byte[] bytes = IOUtil.getBytes( fis );
				string = new String( bytes, "UTF-8" );
			} catch ( IOException e ) {
				logger.log( Level.SEVERE, e.getMessage(), e );
				string = "File control error.";
			}

		} catch ( FileNotFoundException e ) {
			string = "There is no license info.";
		} finally {
			IOUtil.close( fis );
		}

		return string;

	}

	protected void close() {
		logger.info("close the license dialog");
	}
}
