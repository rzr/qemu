/**
 * Screenshot Window
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Munkyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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

package org.tizen.emulator.skin.screenshot;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.tizen.emulator.skin.EmulatorSkin;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.exception.ScreenShotException;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.image.ImageRegistry.IconName;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.util.IOUtil;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.StringUtil;
import org.tizen.emulator.skin.util.SwtUtil;

public class ScreenShotDialog {
	private final static String DETAIL_SCREENSHOT_WINDOW_TITLE = "Screen Shot";

	private final static String DEFAULT_FILE_EXTENSION = "png";
	private static final int CANVAS_MARGIN = 30;
	private static final int TOOLITEM_COOLTIME = 200;
	private static final double MIN_SCALE_FACTOR = 12.5;
	private static final double MAX_SCALE_FACTOR = 800;

	private static Logger logger =
			SkinLogger.getSkinLogger(ScreenShotDialog.class).getLogger();

	protected EmulatorSkin skin;
	protected EmulatorConfig config;
	private Shell shell;

	private ScrolledComposite scrollComposite;
	protected Canvas canvasShot;
	protected Image imageShot;
	protected boolean isCanvasDragging;
	protected Point canvasGrabPosition;

	private Composite statusComposite;
	private Label labelResolution;
	private Label labelScale;

	private ToolItem refreshItem;
	private ToolItem copyItem;
	private ToolItem zoomInItem;
	private ToolItem zoomOutItem;
	private double scaleLevel;

	/**
	 * @brief constructor
	 * @param Image icon : screenshot window icon resource
	*/
	public ScreenShotDialog(final EmulatorSkin skin,
			EmulatorConfig config, Image icon) {
		this.skin = skin;
		this.config = config;

		this.canvasGrabPosition = new Point(-1, -1);
		this.scaleLevel = 100;

		if (SwtUtil.isMacPlatform() == false) {
			shell = new Shell(skin.getShell(), SWT.SHELL_TRIM);
		} else {
			shell = new Shell(skin.getShell().getDisplay(), SWT.SHELL_TRIM);
		}
		shell.setText(DETAIL_SCREENSHOT_WINDOW_TITLE
				+ " - " + SkinUtil.makeEmulatorName(config));

		/* To prevent the icon switching on Mac */
		if (SwtUtil.isMacPlatform() == false) {
			if (icon != null) {
				shell.setImage(icon);
			}
		}

		GridLayout gridLayout = new GridLayout();
		gridLayout.marginWidth = gridLayout.marginHeight = 0;
		gridLayout.horizontalSpacing = gridLayout.verticalSpacing = 0;
		shell.setLayout(gridLayout);

		shell.addListener(SWT.Close, new Listener() {
			@Override
			public void handleEvent(Event event) {
				logger.info("ScreenShot Window is closed");

				if (null != imageShot) {
					imageShot.dispose();
				}

				skin.screenShotDialog = null;
			}
		});

		/* tool bar */
		createToolBar(shell);

		/* screenshot canvas */
		scrollComposite = new ScrolledComposite(shell, SWT.V_SCROLL | SWT.H_SCROLL);
		GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		scrollComposite.setLayoutData(gridData);

		scrollComposite.setExpandHorizontal(true);
		scrollComposite.setExpandVertical(true);

		canvasShot = new Canvas(scrollComposite, SWT.FOCUSED);
		canvasShot.setBackground(shell.getDisplay().getSystemColor(SWT.COLOR_DARK_GRAY));

		scrollComposite.setContent(canvasShot);

		/* status bar */
		createStatusBar(shell);

		shell.pack();
		adjustWindow(skin);

		createCanvasListener(canvasShot);
	}

	private void adjustWindow(EmulatorSkin skin) {
		final Rectangle monitorBounds = Display.getDefault().getBounds();
		logger.info("host monitor display bounds : " + monitorBounds);
		final Rectangle emulatorBounds = skin.getShell().getBounds();
		logger.info("current Emulator window bounds : " + emulatorBounds);
		Rectangle dialogBounds = shell.getBounds();
		logger.info("current ScreenShot window bounds : " + dialogBounds);

		/* size correction */
		shell.setSize(emulatorBounds.width, emulatorBounds.height);
		dialogBounds = shell.getBounds();
		logger.info("current ScreenShot Dialog bounds : " + dialogBounds);

		/* location correction */
		int x = emulatorBounds.x + emulatorBounds.width + 20;
		int y = emulatorBounds.y;
		if ((x + dialogBounds.width) > (monitorBounds.x + monitorBounds.width)) {
			x = emulatorBounds.x - dialogBounds.width - 20;
		}
		if (y < monitorBounds.y) {
			y = monitorBounds.y;
		} else if ((y + dialogBounds.height) > (monitorBounds.y + monitorBounds.height)) {
			y = (monitorBounds.y + monitorBounds.height) - dialogBounds.height;
		}
		shell.setLocation(x, y);

		logger.info("ScreenShot window bounds : " + shell.getBounds());
	}

	private void createCanvasListener(Canvas canvas) {
		canvas.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e) {
				logger.fine("draw shot");

				if (null != imageShot && !imageShot.isDisposed()) {
					e.gc.setInterpolation(SWT.NONE);

					final Rectangle imageBounds = imageShot.getBounds();
					e.gc.drawImage(imageShot, 0, 0, imageBounds.width, imageBounds.height,
							CANVAS_MARGIN, CANVAS_MARGIN,
							(int)(imageBounds.width  * scaleLevel / 100),
							(int)(imageBounds.height * scaleLevel / 100));
				}
			}
		});

		canvas.addMouseMoveListener(new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (isCanvasDragging == true) {
					Point origin = scrollComposite.getOrigin();
					origin.x += canvasGrabPosition.x - e.x;
					origin.y += canvasGrabPosition.y - e.y;
					scrollComposite.setOrigin(origin);
				}
			}
		});

		canvas.addMouseListener(new MouseListener() {
			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (1/* left button */ == e.button) {
					isCanvasDragging = true;
					canvasGrabPosition.x = e.x;
					canvasGrabPosition.y = e.y;
				}
			}

			@Override
			public void mouseUp(MouseEvent e) {
				if (1/* left button */ == e.button) {
					isCanvasDragging = false;
					canvasGrabPosition.x = -1;
					canvasGrabPosition.y = -1;
				}
			}
		});
	}

	private void clickShutter() throws ScreenShotException {
		capture();

		/* set as 100% view */
		setScaleLevel(100);

		shell.getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				updateWindow();
			}
		});
	}

	protected void capture() throws ScreenShotException {
		/* abstract */
	}

	private double getScaleLevel() {
		return scaleLevel;
	}

	private void setScaleLevel(double level) {
		scaleLevel = level;
		logger.info("set scaling level : " + scaleLevel);
	}

	private void downScaleLevel() {
		scaleLevel /= 2;
		logger.info("down scaling level : " + scaleLevel);
	}

	private void upScaleLevel() {
		scaleLevel *= 2;
		logger.info("up scaling level : " + scaleLevel);
	}

	private void updateWindow() {
		logger.info("update");

		if (imageShot != null) {
			ImageData imageData = imageShot.getImageData();
			int width = (int)(imageData.width * scaleLevel / 100) + (2 * CANVAS_MARGIN);
			int height = (int)(imageData.height * scaleLevel / 100) + (2 * CANVAS_MARGIN);
			logger.info("update composite width : " + width + ", height : " + height);

			scrollComposite.setMinSize(width, height);
		}

		/* update tool bar */
		if (zoomInItem != null && zoomOutItem != null) {
			if (getScaleLevel() >= MAX_SCALE_FACTOR) {
				zoomInItem.setEnabled(false);
				zoomOutItem.setEnabled(true);
			} else if (getScaleLevel() <= MIN_SCALE_FACTOR) {
				zoomOutItem.setEnabled(false);
				zoomInItem.setEnabled(true);
			} else {
				zoomInItem.setEnabled(true);
				zoomOutItem.setEnabled(true);
			}
		}

		/* update image */
		canvasShot.redraw();

		/* update status bar */
		if (labelScale != null) {
			labelScale.setText(" " + scaleLevel + "% ");
			labelScale.update();
		}
	}

	protected ImageData rotateImageData(ImageData srcData, RotationInfo rotation) {
		int direction = SWT.NONE;

		switch (rotation) {
		case PORTRAIT:
			return srcData;
		case LANDSCAPE:
			direction = SWT.LEFT;
			break;
		case REVERSE_PORTRAIT:
			direction = SWT.DOWN;
			break;
		case REVERSE_LANDSCAPE:
			direction = SWT.RIGHT;
			break;
		default:
			return srcData;
		}

		ImageData rotatedData = rotateImageData(srcData, direction);
		return rotatedData;
	}

	/*
	 * refrence web page : http://www.java2s.com/Code/Java/SWT-JFace-Eclipse/Rotateandflipanimage.htm
	 */
	private ImageData rotateImageData(ImageData srcData, int direction) {
		int bytesPerPixel = srcData.bytesPerLine / srcData.width;
		int destBytesPerLine = (direction == SWT.DOWN) ?
				srcData.width * bytesPerPixel : srcData.height * bytesPerPixel;

		byte[] newData = new byte[srcData.data.length];
		int srcWidth = 0, srcHeight = 0;

		int destX = 0, destY = 0, destIndex = 0, srcIndex = 0;
		for (int srcY = 0; srcY < srcData.height; srcY++) {
			for (int srcX = 0; srcX < srcData.width; srcX++) {
				switch (direction) {
				case SWT.LEFT: /* left 90 degrees */
					destX = srcY;
					destY = srcData.width - srcX - 1;
					srcWidth = srcData.height;
					srcHeight = srcData.width;
					break;
				case SWT.RIGHT: /* right 90 degrees */
					destX = srcData.height - srcY - 1;
					destY = srcX;
					srcWidth = srcData.height;
					srcHeight = srcData.width;
					break;
				case SWT.DOWN: /* 180 degrees */
					destX = srcData.width - srcX - 1;
					destY = srcData.height - srcY - 1;
					srcWidth = srcData.width;
					srcHeight = srcData.height;
					break;
				}

				destIndex = (destY * destBytesPerLine) + (destX * bytesPerPixel);
				srcIndex = (srcY * srcData.bytesPerLine) + (srcX * bytesPerPixel);
				System.arraycopy(srcData.data, srcIndex, newData, destIndex, bytesPerPixel);
			}
		}

		/* destBytesPerLine is used as scanlinePad
		 * to ensure that no padding is required */
		return new ImageData(srcWidth, srcHeight,
				srcData.depth, srcData.palette, destBytesPerLine, newData);
	}

	protected RotationInfo getCurrentRotation() {
		short currentRotationId =
				skin.getEmulatorSkinState().getCurrentRotationId();
		RotationInfo rotationInfo = RotationInfo.getValue(currentRotationId);

		return rotationInfo;
	}

	private void createToolBar(final Shell parent) {
		ImageRegistry imageRegistry = skin.getImageRegistry();

		ToolBar toolBar = new ToolBar(parent, SWT.HORIZONTAL | SWT.BORDER);
		GridData gridData = new GridData(
				GridData.FILL_HORIZONTAL, GridData.CENTER, true, false);
		toolBar.setLayoutData(gridData);

		/* save */
		ToolItem saveItem = new ToolItem(toolBar, SWT.FLAT);
		saveItem.setImage(imageRegistry.getIcon(IconName.SAVE_SCREEN_SHOT));
		saveItem.setToolTipText("Save to file");

		saveItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				FileDialog fileDialog = new FileDialog(parent, SWT.SAVE);
				fileDialog.setText("Save Image");

				String[] filter = { "*.png;*.PNG;*.jpg;*.JPG;*.jpeg;*.JPEG;*.bmp;*.BMP" };
				fileDialog.setFilterExtensions(filter);

				String[] filterName = { "Image files (*.png *.jpg *.jpeg *.bmp)" };
				fileDialog.setFilterNames(filterName);

				String vmName = SkinUtil.getVmName(config);
				SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-hhmmss");
				String dateString = formatter.format(new Date(System.currentTimeMillis()));

				fileDialog.setFileName(vmName + "-" +
						dateString + "." + DEFAULT_FILE_EXTENSION);

				String userHome = System.getProperty("user.home");
				if (!StringUtil.isEmpty(userHome)) {
					fileDialog.setFilterPath(userHome);
				} else {
					logger.warning("Cannot find user home path in java System properties.");
				}

				fileDialog.setOverwrite(true);
				String filePath = fileDialog.open();
				saveFile(filePath, fileDialog);
			}
		});

		/* copy to clipboard */
		copyItem = new ToolItem(toolBar, SWT.FLAT);
		copyItem.setImage(imageRegistry.getIcon(IconName.COPY_SCREEN_SHOT));
		copyItem.setToolTipText("Copy to clipboard");

		copyItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (null == imageShot || imageShot.isDisposed()) {
					SkinUtil.openMessage(parent, null,
							"Fail to copy to clipboard.", SWT.ICON_ERROR, config);
					return;
				}

				copyItem.setEnabled(false);
				parent.getDisplay().asyncExec(new Runnable() {
					@Override
					public void run() {
						try {
							Thread.sleep(TOOLITEM_COOLTIME);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
						copyItem.setEnabled(true);
					}
				});

				ImageLoader loader = new ImageLoader();
				ImageData shotData = imageShot.getImageData();

				if (SwtUtil.isWindowsPlatform()) {
					/* convert to RGBA */
					shotData.palette =
							new PaletteData(0xFF000000, 0x00FF0000, 0x0000FF00);
				}

				loader.data = new ImageData[] { shotData };

				ByteArrayOutputStream bao = new ByteArrayOutputStream();
				loader.save(bao, SWT.IMAGE_PNG);

				ImageData pngData = new ImageData(
						new ByteArrayInputStream(bao.toByteArray()));
				Object[] imageObject = new Object[] { pngData };

				Transfer[] transfer = new Transfer[] { ImageTransfer.getInstance() };
				Clipboard clipboard = new Clipboard(parent.getDisplay());
				clipboard.setContents(imageObject, transfer);
			}
		});

		new ToolItem(toolBar, SWT.SEPARATOR);

		/* refresh */
		refreshItem = new ToolItem(toolBar, SWT.FLAT);
		refreshItem.setImage(imageRegistry.getIcon(IconName.REFRESH_SCREEN_SHOT));
		refreshItem.setToolTipText("Refresh image");

		refreshItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				refreshItem.setEnabled(false);

				parent.getDisplay().asyncExec(new Runnable() {
					@Override
					public void run() {
						try {
							Thread.sleep(TOOLITEM_COOLTIME);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}						
						refreshItem.setEnabled(true);
					}
				});

				try {
					clickShutter();
				} catch (ScreenShotException ex) {
					logger.log(Level.SEVERE, "Fail to create a screen shot.", ex);
					SkinUtil.openMessage(parent, null,
							"Fail to create a screen shot.", SWT.ERROR, config);
				}
			}
		});

		new ToolItem(toolBar, SWT.SEPARATOR);

		/* zoom in */
		zoomInItem = new ToolItem(toolBar, SWT.FLAT);
		zoomInItem.setImage(imageRegistry.getIcon(IconName.INCREASE_SCALE));
		zoomInItem.setToolTipText("Zoom in");

		zoomInItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				upScaleLevel();

				updateWindow();
			}
		});

		/* zoom out */
		zoomOutItem = new ToolItem(toolBar, SWT.FLAT);
		zoomOutItem.setImage(imageRegistry.getIcon(IconName.DECREASE_SCALE));
		zoomOutItem.setToolTipText("Zoom out");

		zoomOutItem.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				downScaleLevel();

				updateWindow();
			}
		});
	}

	private void createStatusBar(final Shell parent) {
		statusComposite = new Composite(parent, SWT.NONE);
		RowLayout row = new RowLayout(SWT.HORIZONTAL);
		row.marginLeft = 0;
		row.marginTop = row.marginBottom = 0;
		statusComposite.setLayout(row);

		labelResolution = new Label(statusComposite, SWT.BORDER | SWT.SHADOW_IN);
		labelResolution.setText(" Resolution : " +
				skin.getEmulatorSkinState().getCurrentResolutionWidth() + "x" +
				skin.getEmulatorSkinState().getCurrentResolutionHeight() + " ");

		labelScale = new Label(statusComposite, SWT.BORDER | SWT.SHADOW_IN);
		labelScale.setText(" " + scaleLevel + "% ");
	}

	private void saveFile(String fileFullPath, FileDialog fileDialog) {
		if (null == fileFullPath) {
			return;
		}

		String format = "";
		String[] split = fileFullPath.split("\\.");

		if (1 < split.length) {
			format = split[split.length - 1];

			if (new File(split[split.length - 2]).isDirectory()) {
				/* There is no file name */
				SkinUtil.openMessage(shell, null,
						"Use correct file name.", SWT.ICON_WARNING, config);

				String path = fileDialog.open();
				saveFile(path, fileDialog);
			}
		}

		FileOutputStream fos = null;

		try {
			if (StringUtil.isEmpty(format)) {
				if (fileFullPath.endsWith(".")) {
					fileFullPath += DEFAULT_FILE_EXTENSION;
				} else {
					fileFullPath += "." + DEFAULT_FILE_EXTENSION;
				}
			}

			ImageLoader loader = new ImageLoader();
			loader.data = new ImageData[] { imageShot.getImageData() };

			if (StringUtil.isEmpty(format) || format.equalsIgnoreCase("png")) {
				fos = new FileOutputStream(fileFullPath, false);
				loader.save(fos, SWT.IMAGE_PNG);
			} else if (format.equalsIgnoreCase("jpg") || format.equalsIgnoreCase("jpeg")) {
				fos = new FileOutputStream(fileFullPath, false);
				loader.save(fos, SWT.IMAGE_JPEG);
			} else if (format.equalsIgnoreCase("bmp")) {
				fos = new FileOutputStream(fileFullPath, false);
				loader.save(fos, SWT.IMAGE_BMP);
			} else {
				SkinUtil.openMessage(shell, null,
						"Use the specified image formats. (PNG / JPG / JPEG / BMP)",
						SWT.ICON_WARNING, config);

				String path = fileDialog.open();
				saveFile(path, fileDialog);
			}
		} catch (FileNotFoundException ex) {
			logger.log(Level.WARNING, "Use correct file name.", ex);
			SkinUtil.openMessage(shell, null,
					"Use correct file name.", SWT.ICON_WARNING, config);

			String path = fileDialog.open();
			saveFile(path, fileDialog);
		} catch (Exception ex) {
			logger.log(Level.SEVERE, "Fail to save this image file.", ex);
			SkinUtil.openMessage(shell, null,
					"Fail to save this image file.", SWT.ERROR, config);

			String path = fileDialog.open();
			saveFile(path, fileDialog);
		} finally {
			IOUtil.close(fos);
		}
	}

	public void open() throws ScreenShotException {
		if (shell.isDisposed()) {
			return;
		}

		try {
			clickShutter();
		} catch (ScreenShotException e) {
			shell.close();

			throw e;
		}

		shell.open();
	}

	public Shell getShell() {
		return shell;
	}
}
