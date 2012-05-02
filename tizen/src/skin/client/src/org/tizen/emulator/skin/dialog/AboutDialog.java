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
	
	public static final String PROP_KEY_VERSION = "version";
	public static final String PROP_KEY_BUILD_TIME = "build_time";
	public static final String PROP_KEY_GIT_VERSION = "build_git_commit";
	
	private Logger logger = SkinLogger.getSkinLogger( AboutDialog.class ).getLogger();

	public AboutDialog( Shell parent ) {
		super( parent, "About Tizen Emulator", SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL );
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

		Properties properties = getProperties();
		
		Text versionText = new Text( composite, SWT.NONE );
		String version = getValue( properties, PROP_KEY_VERSION );
		versionText.setText( "Version" + "        : " + version );
		versionText.setEditable( false );
		versionText.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WIDGET_BACKGROUND ) );

		Text buildText = new Text( composite, SWT.NONE );
		String time = getValue( properties, PROP_KEY_BUILD_TIME );
		buildText.setText( "Build time" + "  : " + time + " (GMT)" );
		buildText.setEditable( false );
		buildText.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WIDGET_BACKGROUND ) );

		Text gitText = new Text( composite, SWT.NONE );
		String gitVersion = getValue( properties, PROP_KEY_GIT_VERSION );
		gitText.setText( "Git version" + " : " + gitVersion );
		gitText.setEditable( false );
		gitText.setBackground( shell.getDisplay().getSystemColor( SWT.COLOR_WIDGET_BACKGROUND ) );

		return composite;

	}

	private String getValue( Properties properties, String key ) {
		
		if( null != properties ) {

			String property = properties.getProperty( key );
			
			if( !StringUtil.isEmpty( property ) ) {
				
				if( !property.contains( key ) ) {
					return property;
				}else {
					// ex) '${build_git_commit}' is default expression in build.xml
					return "Not identified";
				}
				
			}else {
				return "Not identified";
			}

		}else {
			return "Not identified";
		}
		
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
					LicenseDialog licenseDialog = new LicenseDialog( shell, "License" );
					licenseDialog.open();
					isOpen = false;
				}
			}
		} );

		createOKButton( parent, true );

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
