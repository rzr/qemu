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
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.StringUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class AboutDialog extends SkinDialog {

	public static final String ABOUT_PROP_FILENAME = "about.properties";

	public static final String PROP_KEY_VERSION = "version";
	public static final String PROP_KEY_BUILD_TIME = "build_time";
	public static final String PROP_KEY_GIT_VERSION = "build_git_commit";

	public static final String URL_TIZEN_ORG = "https://developer.tizen.org";

	private Image aboutImage;

	private Logger logger = SkinLogger.getSkinLogger(AboutDialog.class).getLogger();

	public AboutDialog(Shell parent) {
		super(parent, "About Tizen Emulator", SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL);
	}

	private GridLayout getNopaddedGridLayout(int numColumns, boolean makeColumnEqualWidth) {
		GridLayout layout = new GridLayout(numColumns, makeColumnEqualWidth);
		layout.marginLeft = layout.marginRight = 0;
		layout.marginTop = layout.marginBottom = 0;
		layout.marginWidth = layout.marginHeight = 0;
		layout.horizontalSpacing = layout.verticalSpacing = 0;

		return layout;
	}

	@Override
	protected void createComposite() {
		Composite parent = new Composite(shell, SWT.NONE);
		parent.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		GridLayout gridLayout = getNopaddedGridLayout(1, true);
		parent.setLayout(gridLayout);

		Composite area = createArea(parent);
		if (null == area) {
			return;
		}

		/* bottom side */
		buttonComposite = new Composite(parent, SWT.NONE);
		buttonComposite.setLayoutData(new GridData(SWT.RIGHT, SWT.BOTTOM, true, true));
		buttonComposite.setLayout(new GridLayout(1, false));

		createButtons(buttonComposite);
	}

	@Override
	protected Composite createArea(Composite parent) {
		Composite composite = displayInfo(parent);
		return composite;
	}

	private Composite displayInfo(Composite parent) {
		Composite compositeBase = new Composite(parent, SWT.BORDER);
		compositeBase.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
		compositeBase.setLayout(getNopaddedGridLayout(2, false));
		Color white = Display.getCurrent().getSystemColor(SWT.COLOR_WHITE);
		compositeBase.setBackground(white);

		/* left side */
		Composite compositeLeft = new Composite(compositeBase, SWT.BORDER);
		compositeLeft.setLayout(getNopaddedGridLayout(1, false));

		Label imageLabel = new Label(compositeLeft, SWT.NONE);
		aboutImage = new Image(shell.getDisplay(),
				this.getClass().getClassLoader().getResourceAsStream("images/about_Tizen_SDK.png"));
		imageLabel.setImage(aboutImage);

		/* right side */
		Composite compositeRight = new Composite(compositeBase, SWT.NONE);
		compositeRight.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false));
		GridLayout layout = getNopaddedGridLayout(1, true);
		layout.marginLeft = layout.marginRight = 6;
		compositeRight.setLayout(layout);
		compositeRight.setBackground(white);

		/* SDK name */
		Label titleLabel = new Label(compositeRight, SWT.NONE);
		/*GridData gridData = new GridData();
		gridData.grabExcessHorizontalSpace = true;
		gridData.horizontalAlignment = SWT.CENTER;
		titleLabel.setLayoutData(gridData);*/
		titleLabel.setText("\nTizen SDK\n");
		/*Font systemFont = shell.getDisplay().getSystemFont();
		FontData[] fontData = systemFont.getFontData();
		fontData[0].setStyle(SWT.BOLD);
		fontData[0].setHeight(16);
		Font font = new Font(shell.getDisplay(), fontData[0]);
		titleLabel.setFont(font);
		font.dispose();*/
		titleLabel.setBackground(white);

		if (SwtUtil.isWindowsPlatform()) {
			Label space = new Label(compositeRight, SWT.NONE);
			space.setText("\n");
		}

		Properties properties = getProperties();

		/* SDK version */
		Text versionText = new Text(compositeRight, SWT.NONE);
		String version = getValue(properties, PROP_KEY_VERSION);
		if (version.isEmpty()) {
			version = "Not identified";
		}
		versionText.setText("Version : " + version);
		versionText.setEditable(false);
		versionText.setBackground(white);

		/* build id */
		Text buildText = new Text(compositeRight, SWT.NONE);
		String time = getValue(properties, PROP_KEY_BUILD_TIME);
		if (time.isEmpty()) {
			time = "Not identified";
		}
		buildText.setText("Build id : " + time);
		buildText.setEditable(false);
		buildText.setBackground(white);

		/*Text gitText = new Text(compositeRight, SWT.NONE);
		String gitVersion = getValue(properties, PROP_KEY_GIT_VERSION);
		gitText.setText("Git version" + " : " + gitVersion);
		gitText.setEditable(false);
		gitText.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND));*/

		/* link to URL */
		Link visit = new Link(compositeRight, SWT.NONE);
		visit.setText("\n\n Visit <a>" + URL_TIZEN_ORG + "</a>");
		visit.setBackground(white);
		visit.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent arg0) {
				//do nothing
			}

			@Override
			public void widgetSelected(SelectionEvent arg0) {
				Program.launch(URL_TIZEN_ORG);
			}

		});

		return compositeBase;
	}

	private String getValue(Properties properties, String key) {
		if (null != properties) {

			String property = properties.getProperty(key);

			if (!StringUtil.isEmpty(property)) {

				if (!property.contains(key)) {
					return property;
				} else {
					// ex) '${build_git_commit}' is default
					// expression in build.xml
					return "Not identified";
				}

			} else {
				return "Not identified";
			}

		} else {
			return "Not identified";
		}
	}

	@Override
	protected void createButtons(Composite parent) {
		super.createButtons(parent);

		Composite composite = new Composite(parent, SWT.NONE);
		FillLayout fillLayout = new FillLayout(SWT.HORIZONTAL);
		composite.setLayout(fillLayout);

		Button licenseButton = createButton(composite, "License");

		licenseButton.addSelectionListener(new SelectionAdapter() {
			private boolean isOpen;

			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!isOpen) {
					isOpen = true;
					LicenseDialog licenseDialog = new LicenseDialog(shell, "License");
					licenseDialog.open();
					isOpen = false;
				}
			}
		});

		createOKButton(composite, true);
	}

	private Properties getProperties() {
		InputStream is = null;
		Properties properties = null;

		try {
			is = this.getClass().getClassLoader().getResourceAsStream(ABOUT_PROP_FILENAME);
			if (null == is) {
				logger.severe("about properties file is null.");
				return null;
			}

			properties = new Properties();
			properties.load(is);

		} catch (IOException e) {
			logger.log(Level.SEVERE, e.getMessage(), e);
			return null;
		} finally {
			IOUtil.close(is);
		}

		return properties;
	}

	protected void setShellSize() {
		shell.setSize(436, shell.getSize().y);
	}

	protected void close() {
		logger.info("close the about dialog");
		aboutImage.dispose();
	}
}
