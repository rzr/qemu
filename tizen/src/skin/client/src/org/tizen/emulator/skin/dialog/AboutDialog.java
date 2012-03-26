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

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.StringUtil;

public class AboutDialog extends SkinDialog {
	
	public static final String ABOUT_PROP_FILENAME = "about.properties";
	
	private Logger logger = SkinLogger.getSkinLogger( AboutDialog.class ).getLogger();

	public AboutDialog( Shell parent, int style ) {
		super( parent, "About Tizen Emulator", style );
	}

	@Override
	protected Composite createArea( Composite parent ) {
		Composite composite = displayInfo( parent );
		return composite;
	}

	private Composite displayInfo( Composite parent ) {

		Composite composite = new Composite( parent, SWT.NONE );

		composite.setLayout( new GridLayout( 1, false ) );

		Label titleLabel = new Label( composite, SWT.NONE );
		GridData gridData = new GridData();
		gridData.grabExcessHorizontalSpace = true;
		gridData.horizontalAlignment = SWT.CENTER;
		titleLabel.setLayoutData( gridData );

		titleLabel.setText( "Tizen Emulator" );
		Font systemFont = shell.getDisplay().getSystemFont();
		FontData[] fontData = systemFont.getFontData();
		fontData[0].setStyle( SWT.BOLD );
		fontData[0].setHeight( 16 );
		Font font = new Font( shell.getDisplay(), fontData[0] );
		titleLabel.setFont( font );
		font.dispose();

		boolean noInformation = false;
		Properties properties = getProperties();
		if ( null == getProperties() ) {
			noInformation = true;
		}
		
		String defaultInformation = "no information";
		
		Text versionText = new Text( composite, SWT.NONE );
		if( noInformation ) {
			versionText.setText( "Version      : " + defaultInformation );
		}else {
			versionText.setText( "Version      : Tizen SDK " + StringUtil.nvl( properties.getProperty( "version" ) ) );
		}
		versionText.setEditable( false );
		versionText.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WIDGET_BACKGROUND ) );

		Text buildText = new Text( composite, SWT.NONE );
		if( noInformation ) {
			buildText.setText( "Build time  : " + defaultInformation );
		}else {
			buildText.setText( "Build time  : " + StringUtil.nvl( properties.getProperty( "build_time" ) ) + " (GMT)" );
		}
		buildText.setEditable( false );
		buildText.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WIDGET_BACKGROUND ) );

		Text gitText = new Text( composite, SWT.NONE );
		if( noInformation ) {
			gitText.setText( "Git version : " + defaultInformation );
		}else {
			gitText.setText( "Git version : " + StringUtil.nvl( properties.getProperty( "build_git_commit" ) ) );
		}
		gitText.setEditable( false );
		gitText.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WIDGET_BACKGROUND ) );

		return composite;

	}

	@Override
	protected void createButtons( Composite parent ) {

		super.createButtons( parent );

		Button licenseButton = createButton( parent, "License" );
		licenseButton.addSelectionListener( new SelectionAdapter() {
			private boolean isOpen;

			@Override
			public void widgetSelected( SelectionEvent e ) {
				if ( !isOpen ) {
					isOpen = true;
					LicenseDialog licenseDialog = new LicenseDialog( shell, "License", SWT.DIALOG_TRIM );
					licenseDialog.open();
					isOpen = false;
				}
			}
		} );

		createCloseButton( parent, true );

	}

	private Properties getProperties() {

		InputStream is = null;
		Properties properties = null;
		try {

			is = this.getClass().getClassLoader().getResourceAsStream( ABOUT_PROP_FILENAME );
			if ( null == is ) {
				logger.severe( "about properties file is null." );
				return null;
			}

			properties = new Properties();
			properties.load( is );

		} catch ( IOException e ) {
			logger.log( Level.SEVERE, e.getMessage(), e );
			return null;
		} finally {
			IOUtil.close( is );
		}

		return properties;

	}

}
