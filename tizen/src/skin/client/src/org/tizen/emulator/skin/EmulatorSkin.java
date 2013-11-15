/**
 * Emulator Skin Process
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
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

package org.tizen.emulator.skin;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MenuDetectEvent;
import org.eclipse.swt.events.MenuDetectListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseWheelListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.ShellEvent;
import org.eclipse.swt.events.ShellListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.tizen.emulator.skin.comm.ICommunicator.KeyEventType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseButtonType;
import org.tizen.emulator.skin.comm.ICommunicator.MouseEventType;
import org.tizen.emulator.skin.comm.ICommunicator.RotationInfo;
import org.tizen.emulator.skin.comm.ICommunicator.Scale;
import org.tizen.emulator.skin.comm.ICommunicator.SendCommand;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator;
import org.tizen.emulator.skin.comm.sock.SocketCommunicator.DataTranfer;
import org.tizen.emulator.skin.comm.sock.data.BooleanData;
import org.tizen.emulator.skin.comm.sock.data.DisplayStateData;
import org.tizen.emulator.skin.comm.sock.data.KeyEventData;
import org.tizen.emulator.skin.comm.sock.data.MouseEventData;
import org.tizen.emulator.skin.config.EmulatorConfig;
import org.tizen.emulator.skin.config.EmulatorConfig.ArgsConstants;
import org.tizen.emulator.skin.config.EmulatorConfig.SkinPropertiesConstants;
import org.tizen.emulator.skin.custom.CustomProgressBar;
import org.tizen.emulator.skin.custom.SkinWindow;
import org.tizen.emulator.skin.dbi.HoverType;
import org.tizen.emulator.skin.dbi.RgbType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dialog.AboutDialog;
import org.tizen.emulator.skin.dialog.DetailInfoDialog;
import org.tizen.emulator.skin.dialog.RamdumpDialog;
import org.tizen.emulator.skin.image.ImageRegistry;
import org.tizen.emulator.skin.info.EmulatorSkinState;
import org.tizen.emulator.skin.info.SkinInformation;
import org.tizen.emulator.skin.layout.GeneralPurposeSkinComposer;
import org.tizen.emulator.skin.layout.ISkinComposer;
import org.tizen.emulator.skin.layout.ProfileSpecificSkinComposer;
import org.tizen.emulator.skin.log.SkinLogger;
import org.tizen.emulator.skin.menu.KeyWindowKeeper;
import org.tizen.emulator.skin.menu.PopupMenu;
import org.tizen.emulator.skin.screenshot.ScreenShotDialog;
import org.tizen.emulator.skin.util.SkinRotation;
import org.tizen.emulator.skin.util.SkinUtil;
import org.tizen.emulator.skin.util.SwtUtil;

/**
 *
 *
 */
public class EmulatorSkin {
	public enum SkinBasicColor {
		BLUE(0, 174, 239),
		YELLOW(246, 226, 0),
		LIME(0, 246, 12),
		VIOLET(168, 43, 255),
		ORANGE(246, 110, 0),
		MAGENTA(245, 48, 233),
		PURPLE(94, 73, 255),
		GREEN(179, 246, 0),
		RED(245, 48, 48),
		CYON(29, 223, 221);

		private int channelRed;
		private int channelGreen;
		private int channelBlue;

		SkinBasicColor(int r, int g, int b) {
			channelRed = r;
			channelGreen = g;
			channelBlue = b;
		}

		public RGB color() {
			return new RGB(channelRed, channelGreen, channelBlue);
		}
	}

	private static Logger logger =
			SkinLogger.getSkinLogger(EmulatorSkin.class).getLogger();

	protected EmulatorConfig config;
	protected Shell shell;
	protected ImageRegistry imageRegistry;
	protected Canvas lcdCanvas;
	private int displayCanvasStyle;
	public SkinInformation skinInfo;
	protected ISkinComposer skinComposer;

	protected EmulatorSkinState currentState;

	protected boolean isDisplayDragging;
	protected Point shellGrabPosition;
	protected boolean isShutdownRequested;
	public boolean isOnTop;
	public boolean isKeyWindow;
	public boolean isOnKbd;
	private PopupMenu popupMenu;

	public Color colorVM;
	private KeyWindowKeeper keyWindowKeeper;
	public CustomProgressBar bootingProgress;
	public ScreenShotDialog screenShotDialog;

	public SocketCommunicator communicator;
	private ShellListener shellListener;
	private MenuDetectListener shellMenuDetectListener;

	protected boolean isDisplayOn;
	private int prev_x;
	private int prev_y;
	private MouseMoveListener canvasMouseMoveListener;
	private MouseListener canvasMouseListener;
	private MouseWheelListener canvasMouseWheelListener;
	private KeyListener canvasKeyListener;
	private MenuDetectListener canvasMenuDetectListener;
	private FocusListener canvasFocusListener;
	private LinkedList<KeyEventData> pressedKeyEventList;

	/**
	 * @brief constructor
	 * @param config : configuration of emulator skin
	 * @param isOnTop : always on top flag
	 */
	protected EmulatorSkin(EmulatorConfig config, SkinInformation skinInfo,
			int displayCanvasStyle, boolean isOnTop) {
		this.config = config;
		this.skinInfo = skinInfo;

		this.screenShotDialog = null;
		this.pressedKeyEventList = new LinkedList<KeyEventData>();

		this.isOnTop = isOnTop;
		this.isKeyWindow = false;

		int style = SWT.NO_TRIM | SWT.DOUBLE_BUFFERED;
		this.shell = new Shell(Display.getDefault(), style);
		if (isOnTop == true) {
			if (SkinUtil.setTopMost(shell, true) == false) {
				logger.info("failed to set top most");
				this.isOnTop = false;
			}
		}

		this.displayCanvasStyle = displayCanvasStyle;

		/* prepare for VM state management */
		this.shellGrabPosition = new Point(-1, -1);
		this.currentState = new EmulatorSkinState();

		setColorVM(); /* generate a identity color */

		this.keyWindowKeeper = new KeyWindowKeeper(this);
	}

	public void setCommunicator(SocketCommunicator communicator) {
		this.communicator = communicator;
	}

	private void setColorVM() {
		int portNumber = config.getArgInt(ArgsConstants.VM_BASE_PORT) % 100;

		if (portNumber >= 26200) {
			Random rand = new Random();

			int red = rand.nextInt(255);
			int green = rand.nextInt(255);
			int blue = rand.nextInt(255);
			this.colorVM = new Color(shell.getDisplay(), new RGB(red, green, blue));
		} else {
			int vmIndex = (portNumber % 100) / 10;

			SkinBasicColor colors[] = SkinBasicColor.values();
			this.colorVM = new Color(shell.getDisplay(),
					colors[vmIndex].color());
		}
	}

	public long initLayout() {
		logger.info("initialize the skin layout");

		imageRegistry = ImageRegistry.getInstance();

		/* set emulator states */
		currentState.setCurrentResolutionWidth(
				config.getArgInt(ArgsConstants.RESOLUTION_WIDTH));
		currentState.setCurrentResolutionHeight(
				config.getArgInt(ArgsConstants.RESOLUTION_HEIGHT));

		int scale = SkinUtil.getValidScale(config);
		currentState.setCurrentScale(scale);

		short rotationId = EmulatorConfig.DEFAULT_WINDOW_ROTATION;
		currentState.setCurrentRotationId(rotationId);

		/* create and attach a popup menu */
		isOnKbd = false;
		popupMenu = new PopupMenu(config, this);

		/* build a skin layout */
		if (skinInfo.isGeneralPurposeSkin() == false) {
			skinComposer = new ProfileSpecificSkinComposer(config, this);

			((ProfileSpecificSkinComposer) skinComposer)
					.addProfileSpecificListener(shell);
		} else { /* general purpose skin */
			skinComposer = new GeneralPurposeSkinComposer(config, this);

			((GeneralPurposeSkinComposer) skinComposer)
					.addGeneralPurposeListener(shell);
		}

		lcdCanvas = skinComposer.compose(displayCanvasStyle);

		if (config.getArgBoolean(ArgsConstants.INPUT_MOUSE, false) == true) {
			prev_x = lcdCanvas.getSize().x / 2;
			prev_y = lcdCanvas.getSize().y / 2;
			logger.info("prev_x : " + prev_x + " prev_y : " + prev_y);
			isDisplayOn = false;
		}

		/* load a hover color */
		currentState.setHoverColor(loadHoverColor());

		/* added event handlers */
		addMainWindowListeners();
		addCanvasListeners();

		setFocus();

		return 0;
	}

	private Color loadHoverColor() {
		HoverType hover = config.getDbiContents().getHover();

		if (null != hover) {
			RgbType hoverRgb = hover.getColor();
			if (null != hoverRgb) {
				Long r = hoverRgb.getR();
				Long g = hoverRgb.getG();
				Long b = hoverRgb.getB();

				if (null != r && null != g && null != b) {
					Color hoverColor = new Color(shell.getDisplay(), new RGB(
							r.intValue(), g.intValue(), b.intValue()));

					return hoverColor;
				}
			}
		}

		/* white */
		return (new Color(shell.getDisplay(), new RGB(255, 255, 255)));
	}

	/* getters */
	public Shell getShell() {
		return shell;
	}

	public EmulatorSkinState getEmulatorSkinState() {
		return currentState;
	}

	public ImageRegistry getImageRegistry() {
		return imageRegistry;
	}

	public Color getColorVM() {
		return colorVM;
	}

	public PopupMenu getPopupMenu() {
		return popupMenu;
	}

	public KeyWindowKeeper getKeyWindowKeeper() {
		return keyWindowKeeper;
	}

	public void open() {
		if (null == this.communicator) {
			logger.severe("communicator is null.");
			return;
		}

		Display display = this.shell.getDisplay();

		this.shell.open();

		while (!shell.isDisposed()) {
			if (!display.readAndDispatch()) {
				display.sleep();
			}
		}
	}

	protected void skinFinalize() {
		logger.info("skinFinalize");

		skinComposer.composerFinalize();
	}

	/* window grabbing */
	public void grabShell(int x, int y) {
		shellGrabPosition.x = x;
		shellGrabPosition.y = y;
	}

	public void ungrabShell() {
		shellGrabPosition.x = -1;
		shellGrabPosition.y = -1;
	}

	public boolean isShellGrabbing() {
		return shellGrabPosition.x >= 0 && shellGrabPosition.y >= 0;
	}

	public Point getGrabPosition() {
		if (isShellGrabbing() == false) {
			return null;
		}

		return shellGrabPosition;
	}

	private void addMainWindowListeners() {
		shellListener = new ShellListener() {
			@Override
			public void shellClosed(ShellEvent event) {
				logger.info("Main Window is closed");

				if (isShutdownRequested) {
					removeShellListeners();
					removeCanvasListeners();

					/* close the screen shot window */
					if (null != screenShotDialog) {
						Shell scShell = screenShotDialog.getShell();
						if (!scShell.isDisposed()) {
							scShell.close();
						}
						screenShotDialog = null;
					}

					/* save config only for emulator close */
					config.setSkinProperty(
							SkinPropertiesConstants.WINDOW_X,
							shell.getLocation().x);
					config.setSkinProperty(
							SkinPropertiesConstants.WINDOW_Y,
							shell.getLocation().y);
					config.setSkinProperty(
							SkinPropertiesConstants.WINDOW_SCALE,
							currentState.getCurrentScale());
					config.setSkinProperty(
							SkinPropertiesConstants.WINDOW_ROTATION,
							currentState.getCurrentRotationId());
					config.setSkinProperty(
							SkinPropertiesConstants.WINDOW_ONTOP,
							Boolean.toString(isOnTop));

					int dockValue = 0;
					SkinWindow keyWindow = getKeyWindowKeeper().getKeyWindow();
					if (keyWindow != null
							&& keyWindow.getShell().isVisible() == true) {
						dockValue = getKeyWindowKeeper().getDockPosition();
					}
					config.setSkinProperty(
							SkinPropertiesConstants.KEYWINDOW_POSITION,
							dockValue);

					config.saveSkinProperties();

					/* close the Key Window */
					if (getKeyWindowKeeper() != null) {
						getKeyWindowKeeper().dispose();
					}

					/* dispose the images */
					if (currentState.getCurrentImage() != null) {
						currentState.getCurrentImage().dispose();
						currentState.setCurrentImage(null);
					}
					if (currentState.getCurrentKeyPressedImage() != null) {
						currentState.getCurrentKeyPressedImage().dispose();
						currentState.setCurrentKeyPressedImage(null);
					}

					/* dispose the color */
					if (colorVM != null) {
						colorVM.dispose();
					}

					if (currentState.getHoverColor() != null) {
						currentState.getHoverColor().dispose();
					}

					skinFinalize();
				} else {
					/*
					 * Skin have to be alive until receiving shutdown request
					 * from qemu
					 */
					event.doit = false;

					if (null != communicator) {
						communicator.sendToQEMU(SendCommand.SEND_CLOSE_REQ, null, false);
					}
				}
			}

			@Override
			public void shellActivated(ShellEvent event) {
				logger.info("activate");

				if (isKeyWindow == true
						&& getKeyWindowKeeper().getKeyWindow() != null) {
					if (isOnTop == false) {
						getKeyWindowKeeper().getKeyWindow().getShell()
								.moveAbove(shell);

						if (getKeyWindowKeeper().getDockPosition() != SWT.NONE) {
							shell.moveAbove(getKeyWindowKeeper().getKeyWindow()
									.getShell());
						}
					} else {
						if (getKeyWindowKeeper().getDockPosition() == SWT.NONE) {
							getKeyWindowKeeper().getKeyWindow().getShell()
									.moveAbove(shell);
						}
					}
				}
			}

			@Override
			public void shellDeactivated(ShellEvent event) {
				// logger.info("deactivate");

				/* do nothing */
			}

			@Override
			public void shellIconified(ShellEvent event) {
				logger.info("iconified");

				/* synchronization of minimization */
				shell.getDisplay().asyncExec(new Runnable() {
					@Override
					public void run() {
						if (isKeyWindow == true
								&& getKeyWindowKeeper().getKeyWindow() != null) {
							Shell keyWindowShell = getKeyWindowKeeper()
									.getKeyWindow().getShell();

							if (keyWindowShell.getMinimized() == false) {
								keyWindowShell.setVisible(false);
								/*
								 * the tool style window is exposed when even it
								 * was minimized
								 */
								keyWindowShell.setMinimized(true);
							}
						}
					}
				});
			}

			@Override
			public void shellDeiconified(ShellEvent event) {
				logger.info("deiconified");

				if (isKeyWindow == true
						&& getKeyWindowKeeper().getKeyWindow() != null) {
					Shell keyWindowShell = getKeyWindowKeeper().getKeyWindow()
							.getShell();

					if (keyWindowShell.getMinimized() == true) {
						keyWindowShell.setMinimized(false);
						keyWindowShell.setVisible(true);
					}
				}

				DisplayStateData lcdStateData = new DisplayStateData(
						currentState.getCurrentScale(),
						currentState.getCurrentRotationId());
				communicator.sendToQEMU(SendCommand.SEND_DISPLAY_STATE,
						lcdStateData, false);
			}
		};

		shell.addShellListener(shellListener);

		/* menu */
		shellMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				if (isDisplayDragging == true || isShellGrabbing() == true
						|| isShutdownRequested == true) {
					logger.info("menu is blocked");

					e.doit = false;
					return;
				}

				Menu menu = popupMenu.getMenuRoot();
				keyForceRelease(true);

				if (menu != null) {
					shell.setMenu(menu);
					menu.setVisible(true);

					e.doit = false;
				} else {
					shell.setMenu(null);
				}
			}
		};

		shell.addMenuDetectListener(shellMenuDetectListener);
	}

	private void removeShellListeners() {
		if (null != shellListener) {
			shell.removeShellListener(shellListener);
		}

		if (null != shellMenuDetectListener) {
			shell.removeMenuDetectListener(shellMenuDetectListener);
		}
	}

	private MouseMoveListener makeDisplayTouchMoveListener() {
		MouseMoveListener listener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (true == isDisplayDragging) {
					int eventType = MouseEventType.DRAG.value();
					Point canvasSize = lcdCanvas.getSize();

					if (e.x < 0) {
						e.x = 0;
						eventType = MouseEventType.RELEASE.value();
						isDisplayDragging = false;
					} else if (e.x >= canvasSize.x) {
						e.x = canvasSize.x - 1;
						eventType = MouseEventType.RELEASE.value();
						isDisplayDragging = false;
					}

					if (e.y < 0) {
						e.y = 0;
						eventType = MouseEventType.RELEASE.value();
						isDisplayDragging = false;
					} else if (e.y >= canvasSize.y) {
						e.y = canvasSize.y - 1;
						eventType = MouseEventType.RELEASE.value();
						isDisplayDragging = false;
					}

					mouseMoveDelivery(e, eventType);
				}
			}
		};

		return listener;
	}

	private void getRelativePoint(MouseEvent e) {
		int diff_x = e.x - prev_x;
		int diff_y = e.y - prev_y;
		int final_x = e.x;
		int final_y = e.y;
		Point canvasSize = lcdCanvas.getSize();

		/* caculate maximum relative point */
		if (final_x >= canvasSize.x) {
			e.x = canvasSize.x - prev_x - 1;
			prev_x = canvasSize.x - 1;
		} else if (final_x <= 0) {
			e.x = -prev_x;
			prev_x = 0;
		} else {
			prev_x = e.x;
			e.x = diff_x;
		}

		if (final_y >= canvasSize.y) {
			e.y = canvasSize.y - prev_y - 1;
			prev_y = canvasSize.y - 1;
		} else if (final_y <= 0) {
			e.y = -prev_y;
			prev_y = 0;
		} else {
			prev_y = e.y;
			e.y = diff_y;
		}
	}

	private MouseMoveListener makeDisplayMouseMoveListener() {
		MouseMoveListener listener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				if (isDisplayOn) {
					int eventType = MouseEventType.MOVE.value();

					if (true == isDisplayDragging) {
						Point canvasSize = lcdCanvas.getSize();
						eventType = MouseEventType.DRAG.value();
						if (e.x < 0) {
							e.x = 0;
							eventType = MouseEventType.RELEASE.value();
							isDisplayDragging = false;
						} else if (e.x >= canvasSize.x) {
							e.x = canvasSize.x - 1;
							eventType = MouseEventType.RELEASE.value();
							isDisplayDragging = false;
						}

						if (e.y < 0) {
							e.y = 0;
							eventType = MouseEventType.RELEASE.value();
							isDisplayDragging = false;
						} else if (e.y >= canvasSize.y) {
							e.y = canvasSize.y - 1;
							eventType = MouseEventType.RELEASE.value();
							isDisplayDragging = false;
						}
					}
					getRelativePoint(e);

					if (e.x != 0 || e.y != 0)
						mouseMoveDelivery(e, eventType);
				}
			}
		};

		return listener;
	}

	private MouseListener makeDisplayTouchClickListener() {
		MouseListener listener = new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e) {
				getKeyWindowKeeper().redock(false, false);

				if (1 == e.button) /* left button */
				{
					if (true == isDisplayDragging) {
						isDisplayDragging = false;
					}

					mouseUpDelivery(e);
				} else if (2 == e.button) /* wheel button */
				{
					logger.info("wheelUp in display");
				}
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (1 == e.button) /* left button */
				{
					if (false == isDisplayDragging) {
						isDisplayDragging = true;
					}

					mouseDownDelivery(e);
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				/* do nothing */
			}
		};

		return listener;
	}

	private MouseListener makeDisplayMouseClickListener() {
		MouseListener listener = new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e) {
				if (isDisplayOn) {
					logger.info("mouse up : " + e);
					if (1 == e.button) { /* left button */
						if (true == isDisplayDragging) {
							isDisplayDragging = false;
						}
						getRelativePoint(e);
						mouseUpDelivery(e);
					} else if (2 == e.button) { /* wheel button */
						logger.info("wheelUp in display");
					}
				}
			}

			@Override
			public void mouseDown(MouseEvent e) {
				if (isDisplayOn) {
					logger.info("mouse down : " + e);
					if (1 == e.button) { /* left button */
						if (false == isDisplayDragging) {
							isDisplayDragging = true;
						}
						getRelativePoint(e);
						mouseDownDelivery(e);
					}
				}
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				logger.info("mouse double click : " + e);
				// TODO:
			}
		};

		return listener;
	}

	private MouseWheelListener makeDisplayMouseWheelListener() {
		MouseWheelListener listener = new MouseWheelListener() {
			@Override
			public void mouseScrolled(MouseEvent e) {
				if (config.getArgBoolean(ArgsConstants.INPUT_MOUSE, false) == true)
					getRelativePoint(e);

				int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
						currentState.getCurrentResolutionWidth(),
						currentState.getCurrentResolutionHeight(),
						currentState.getCurrentScale(),
						currentState.getCurrentAngle());
				logger.info("mousewheel in display" + " x:" + geometry[0]
						+ " y:" + geometry[1] + " value:" + e.count);

				int eventType;
				if (e.count < 0) {
					eventType = MouseEventType.WHEELDOWN.value();
				} else {
					eventType = MouseEventType.WHEELUP.value();
				}

				MouseEventData mouseEventData = new MouseEventData(
						MouseButtonType.WHEEL.value(), eventType, e.x, e.y,
						geometry[0], geometry[1], e.count);

				communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT,
						mouseEventData, false);
			}
		};

		return listener;
	}

	private void addCanvasListeners() {
		/* menu */
		canvasMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				if (isDisplayDragging == true || isShellGrabbing() == true
						|| isShutdownRequested == true) {
					logger.info("menu is blocked");

					e.doit = false;
					return;
				}

				Menu menu = popupMenu.getMenuRoot();
				keyForceRelease(true);

				if (menu != null) {
					lcdCanvas.setMenu(menu);
					menu.setVisible(true);

					e.doit = false;
				} else {
					lcdCanvas.setMenu(null);
				}
			}
		};

		/* remove 'input method' menu item (avoid bug) */
		lcdCanvas.addMenuDetectListener(canvasMenuDetectListener);

		/* focus */
		canvasFocusListener = new FocusListener() {
			@Override
			public void focusGained(FocusEvent event) {
				// logger.info("gain focus");

				/* do nothing */
			}

			@Override
			public void focusLost(FocusEvent event) {
				logger.info("lost focus");
				keyForceRelease(false);
			}
		};

		lcdCanvas.addFocusListener(canvasFocusListener);

		/* mouse event */
		if (config.getArgBoolean(ArgsConstants.INPUT_MOUSE, false) == true) {
			canvasMouseMoveListener = makeDisplayMouseMoveListener();
		} else {
			canvasMouseMoveListener = makeDisplayTouchMoveListener();
		}
		lcdCanvas.addMouseMoveListener(canvasMouseMoveListener);

		if (config.getArgBoolean(ArgsConstants.INPUT_MOUSE, false) == true) {
			canvasMouseListener = makeDisplayMouseClickListener();
		} else {
			canvasMouseListener = makeDisplayTouchClickListener();
		}
		lcdCanvas.addMouseListener(canvasMouseListener);

		canvasMouseWheelListener = makeDisplayMouseWheelListener();
		lcdCanvas.addMouseWheelListener(canvasMouseWheelListener);

		/* keyboard event */
		canvasKeyListener = new KeyListener() {
			private KeyEvent previous;
			private boolean disappearEvent = false;
			private int disappearKeycode = 0;
			private int disappearStateMask = 0;
			private int disappearKeyLocation = 0;

			@Override
			public void keyReleased(KeyEvent e) {
				if (logger.isLoggable(Level.INFO)) {
					String character =
							(e.character == '\0') ? "\\0" :
							(e.character == '\n') ? "\\n" :
							(e.character == '\r') ? "\\r" :
							("" + e.character);

					logger.info("'" + character + "':"
							+ e.keyCode + ":"
							+ e.stateMask + ":"
							+ e.keyLocation);
				} else if (logger.isLoggable(Level.FINE)) {
					logger.fine(e.toString());
				}

				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				if (SwtUtil.isWindowsPlatform() == true) {
					if (disappearEvent == true) {
						/* generate a disappeared key event in Windows */
						disappearEvent = false;

						if (isMetaKey(keyCode) && e.character != '\0') {
							logger.info("send disappear release : keycode="
									+ disappearKeycode + ", stateMask="
									+ disappearStateMask + ", keyLocation="
									+ disappearKeyLocation);

							KeyEventData keyEventData = new KeyEventData(
									KeyEventType.RELEASED.value(),
									disappearKeycode, disappearStateMask,
									disappearKeyLocation);
							communicator.sendToQEMU(SendCommand.SEND_KEYBOARD_KEY_EVENT,
									keyEventData, false);

							removePressedKeyFromList(keyEventData);

							disappearKeycode = 0;
							disappearStateMask = 0;
							disappearKeyLocation = 0;
						}
					}

					if (previous != null) {
						KeyEventData keyEventData = null;

						/* separate a merged release event */
						if (previous.keyCode == SWT.CR &&
								(keyCode & SWT.KEYCODE_BIT) != 0 && e.character == SWT.CR) {
							logger.info("send upon release : keycode=" + (int)SWT.CR);

							keyEventData = new KeyEventData(
									KeyEventType.RELEASED.value(),
									SWT.CR, 0, 0);
						} else if (previous.keyCode == SWT.SPACE &&
								(keyCode & SWT.KEYCODE_BIT) != 0 &&
								(e.character == SWT.SPACE)) {
							logger.info("send upon release : keycode=" + (int)SWT.SPACE);

							keyEventData = new KeyEventData(
									KeyEventType.RELEASED.value(),
									SWT.SPACE, 0, 0);
						} else if (previous.keyCode == SWT.TAB &&
								(keyCode & SWT.KEYCODE_BIT) != 0 &&
								(e.character == SWT.TAB)) {
							logger.info("send upon release : keycode=" + (int)SWT.TAB);

							keyEventData = new KeyEventData(
									KeyEventType.RELEASED.value(),
									SWT.TAB, 0, 0);
						} else if (previous.keyCode == SWT.KEYPAD_CR &&
								(keyCode & SWT.KEYCODE_BIT) != 0 &&
								(e.character == SWT.CR) &&
								(keyCode != SWT.KEYPAD_CR)) {
							logger.info("send upon release : keycode=" + (int)SWT.KEYPAD_CR);

							keyEventData = new KeyEventData(
									KeyEventType.RELEASED.value(),
									SWT.KEYPAD_CR, 0, 2);
						}

						if (keyEventData != null) {
							communicator.sendToQEMU(SendCommand.SEND_KEYBOARD_KEY_EVENT,
									keyEventData, false);
							removePressedKeyFromList(keyEventData);
						}
					}
				} /* end isWindowsPlatform */

				previous = null;

				keyReleasedDelivery(keyCode, stateMask, e.keyLocation);
			}

			@Override
			public void keyPressed(KeyEvent e) {
				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				/*
				 * When the pressed key event is overwritten by next key event,
				 * the release event of previous key does not occur in Windows.
				 * So, we generate a release key event by ourselves that had
				 * been disappeared.
				 */
				if (SwtUtil.isWindowsPlatform() == true) {
					if (null != previous) {
						if (previous.keyCode != keyCode) {

							if (isMetaKey(previous.keyCode)) {
								disappearEvent = true;
								disappearKeycode = keyCode;
								disappearStateMask = stateMask;
								disappearKeyLocation = e.keyLocation;
							} else {
								/* three or more keys were pressed at the
								 * same time */
								if (disappearEvent == true) {
									logger.info("replace the disappearEvent : "
											+ disappearKeycode + "->" + keyCode);

									disappearKeycode = keyCode;
									disappearStateMask = stateMask;
									disappearKeyLocation = e.keyLocation;
								}

								int previousKeyCode = previous.keyCode;
								int previousStateMask = previous.stateMask;

								if (logger.isLoggable(Level.INFO)) {
									String character =
											(previous.character == '\0') ? "\\0" :
											(previous.character == '\n') ? "\\n" :
											(previous.character == '\r') ? "\\r" :
											("" + previous.character);

									logger.info("send previous release : '"
											+ character + "':"
											+ previous.keyCode + ":"
											+ previous.stateMask + ":"
											+ previous.keyLocation);
								} else if (logger.isLoggable(Level.FINE)) {
									logger.fine("send previous release :"
											+ previous.toString());
								}

								KeyEventData keyEventData = new KeyEventData(
										KeyEventType.RELEASED.value(),
										previousKeyCode, previousStateMask,
										previous.keyLocation);
								communicator.sendToQEMU(
										SendCommand.SEND_KEYBOARD_KEY_EVENT,
										keyEventData, false);

								removePressedKeyFromList(keyEventData);
							}

						}
					}
				} /* end isWindowsPlatform */

				if (logger.isLoggable(Level.INFO)) {
					String character =
							(e.character == '\0') ? "\\0" :
							(e.character == '\n') ? "\\n" :
							(e.character == '\r') ? "\\r" :
							("" + e.character);

					logger.info("'" + character + "':"
							+ e.keyCode + ":"
							+ e.stateMask + ":"
							+ e.keyLocation);
				} else if (logger.isLoggable(Level.FINE)) {
					logger.fine(e.toString());
				}

				keyPressedDelivery(keyCode, stateMask, e.keyLocation);

				previous = e;
			}
		};

		lcdCanvas.addKeyListener(canvasKeyListener);
	}

	/* for display */
	protected void mouseMoveDelivery(MouseEvent e, int eventType) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), eventType, e.x, e.y, geometry[0],
				geometry[1], 0);

		communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData,
				false);
	}

	protected void mouseUpDelivery(MouseEvent e) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());
		logger.info("mouseUp in display" + " x:" + geometry[0] + " y:"
				+ geometry[1]);

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData,
				false);
	}

	protected void mouseDownDelivery(MouseEvent e) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());
		logger.info("mouseDown in display" + " x:" + geometry[0] + " y:"
				+ geometry[1]);

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(SendCommand.SEND_MOUSE_EVENT, mouseEventData,
				false);
	}

	protected void keyReleasedDelivery(int keyCode, int stateMask,
			int keyLocation) {
		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.RELEASED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEYBOARD_KEY_EVENT, keyEventData, false);

		removePressedKeyFromList(keyEventData);
	}

	protected void keyPressedDelivery(int keyCode, int stateMask,
			int keyLocation) {
		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.PRESSED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEYBOARD_KEY_EVENT, keyEventData, false);

		addPressedKeyToList(keyEventData);
	}

	private boolean isMetaKey(int keyCode) {
		if (SWT.CTRL == keyCode || SWT.ALT == keyCode || SWT.SHIFT == keyCode
				|| SWT.COMMAND == keyCode) {
			return true;
		}

		return false;
	}

	protected synchronized boolean addPressedKeyToList(KeyEventData pressData) {
		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == pressData.keycode &&
			// data.stateMask == pressData.stateMask &&
					data.keyLocation == pressData.keyLocation) {
				return false;
			}
		}

		pressedKeyEventList.add(pressData);
		return true;
	}

	protected synchronized boolean removePressedKeyFromList(
			KeyEventData releaseData) {

		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == releaseData.keycode &&
			// data.stateMask == releaseData.stateMask &&
					data.keyLocation == releaseData.keyLocation) {
				pressedKeyEventList.remove(data);

				return true;
			}
		}

		return false;
	}

	private void removeCanvasListeners() {
		if (null != canvasMouseMoveListener) {
			lcdCanvas.removeMouseMoveListener(canvasMouseMoveListener);
		}

		if (null != canvasMouseListener) {
			lcdCanvas.removeMouseListener(canvasMouseListener);
		}

		if (null != canvasKeyListener) {
			lcdCanvas.removeKeyListener(canvasKeyListener);
		}

		if (null != canvasMenuDetectListener) {
			lcdCanvas.removeMenuDetectListener(canvasMenuDetectListener);
		}

		if (null != canvasFocusListener) {
			lcdCanvas.removeFocusListener(canvasFocusListener);
		}

		if (null != canvasMouseWheelListener) {
			lcdCanvas.removeMouseWheelListener(canvasMouseWheelListener);
		}
	}

	public void setFocus() {
		lcdCanvas.setFocus();
	}

	public void updateSkin() {
		skinComposer.updateSkin();
	}

	public void updateDisplay() {
		/* abstract */
	}

	public void drawImageToDisplay(Image image) {
		/* abstract */
	}

	protected void openScreenShotWindow() {
		/* abstract */
	}

	public void updateProgressBar(final int idxLayer, final int progress) {
		getShell().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				if (bootingProgress != null) {
					bootingProgress.setSelection(idxLayer, progress);
				}
			}
		});
	}

	public void updateBrightness(final boolean on) {
		getShell().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				if (on == true) {
					displayOn();
				} else {
					displayOff();
				}
			}
		});
	}

	public void updateHostKbdMenu(final boolean on) {
		isOnKbd = on;

		getShell().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run() {
				getPopupMenu().kbdOnItem.setSelection(isOnKbd);
				getPopupMenu().kbdOffItem.setSelection(!isOnKbd);
			}
		});
	}

	protected void displayOn() {
		logger.info("display on");

		if (config.getArgBoolean(ArgsConstants.INPUT_MOUSE, false) == true) {
			isDisplayOn = true;
		}
	}

	protected void displayOff() {
		logger.info("display off");

		if (config.getArgBoolean(ArgsConstants.INPUT_MOUSE, false) == true) {
			isDisplayOn = false;
		}

		if (isDisplayDragging == true) {
			logger.info("auto release : mouseEvent");
			MouseEventData mouseEventData = new MouseEventData(
					0, MouseEventType.RELEASE.value(),
					0, 0, 0, 0, 0);

			communicator.sendToQEMU(
					SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
		}
	}

	/* for popup menu */
	public SelectionAdapter createDetailInfoMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (logger.isLoggable(Level.FINE)) {
					logger.fine("Open detail info");
				}

				DetailInfoDialog detailInfoDialog = new DetailInfoDialog(
						shell, communicator, config, skinInfo);
				detailInfoDialog.open();
			}
		};

		return listener;
	}

	public SelectionAdapter createTopMostMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				isOnTop = popupMenu.onTopItem.getSelection();

				logger.info("Select Always On Top : " + isOnTop);

				if (SkinUtil.setTopMost(shell, isOnTop) == false) {
					logger.info("failed to Always On Top");

					popupMenu.onTopItem.setSelection(isOnTop = false);
				} else {
					if (getKeyWindowKeeper().getKeyWindow() != null) {
						SkinUtil.setTopMost(getKeyWindowKeeper().getKeyWindow()
								.getShell(), isOnTop);
					}
				}
			}
		};

		return listener;
	}

	public Menu createRotateMenu() {
		Menu menu = new Menu(shell, SWT.DROP_DOWN);

		final List<MenuItem> rotationList = new ArrayList<MenuItem>();

		Iterator<Entry<Short, RotationType>> iterator =
				SkinRotation.getRotationIterator();

		while (iterator.hasNext()) {
			Entry<Short, RotationType> entry = iterator.next();
			Short rotationId = entry.getKey();
			RotationType section = entry.getValue();

			final MenuItem menuItem = new MenuItem(menu, SWT.RADIO);
			menuItem.setText(section.getName().value());
			menuItem.setData(rotationId);

			if (currentState.getCurrentRotationId() == rotationId) {
				menuItem.setSelection(true);
			}

			rotationList.add(menuItem);
		}

		/* temp : swap rotation menu names */
		if (currentState.getCurrentResolutionWidth() > currentState
				.getCurrentResolutionHeight()) {
			for (MenuItem m : rotationList) {
				short rotationId = (Short) m.getData();

				if (rotationId == RotationInfo.PORTRAIT.id()) {
					String landscape = SkinRotation
							.getRotation(RotationInfo.LANDSCAPE.id()).getName()
							.value();
					m.setText(landscape);
				} else if (rotationId == RotationInfo.LANDSCAPE.id()) {
					String portrait = SkinRotation
							.getRotation(RotationInfo.PORTRAIT.id()).getName()
							.value();
					m.setText(portrait);
				} else if (rotationId == RotationInfo.REVERSE_PORTRAIT.id()) {
					String landscapeReverse = SkinRotation
							.getRotation(RotationInfo.REVERSE_LANDSCAPE.id())
							.getName().value();
					m.setText(landscapeReverse);
				} else if (rotationId == RotationInfo.REVERSE_LANDSCAPE.id()) {
					String portraitReverse = SkinRotation
							.getRotation(RotationInfo.REVERSE_PORTRAIT.id())
							.getName().value();
					m.setText(portraitReverse);
				}
			}
		}

		SelectionAdapter selectionAdapter = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if (!selection) {
					return;
				}

				if (!communicator.isSensorDaemonStarted()) {
					/* roll back a selection */
					item.setSelection(false);

					for (MenuItem m : rotationList) {
						short rotationId = (Short) m.getData();
						if (currentState.getCurrentRotationId() == rotationId) {
							m.setSelection(true);
							break;
						}
					}

					SkinUtil.openMessage(shell, null,
							"Rotation is not ready.\n"
									+ "Please wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				final short rotationId = ((Short) item.getData());

				shell.getDisplay().syncExec(new Runnable() {
					@Override
					public void run() {
						skinComposer.arrangeSkin(
								currentState.getCurrentScale(), rotationId);

						/* location correction */
						Rectangle monitorBounds = Display.getDefault().getBounds();
						Rectangle emulatorBounds = shell.getBounds();
						shell.setLocation(
								Math.max(emulatorBounds.x, monitorBounds.x),
								Math.max(emulatorBounds.y, monitorBounds.y));
					}
				});

				DisplayStateData lcdStateData = new DisplayStateData(
						currentState.getCurrentScale(), rotationId);
				communicator.sendToQEMU(SendCommand.SEND_DISPLAY_STATE,
						lcdStateData, false);
			}
		};

		for (MenuItem menuItem : rotationList) {
			menuItem.addSelectionListener(selectionAdapter);
		}

		return menu;
	}

	public Menu createScaleMenuListener() {
		Menu menu = new Menu(shell, SWT.DROP_DOWN);

		final List<MenuItem> scaleList = new ArrayList<MenuItem>();

		final MenuItem scaleOneItem = new MenuItem(menu, SWT.RADIO);
		scaleOneItem.setText("1x");
		scaleOneItem.setData(Scale.SCALE_100);
		scaleList.add(scaleOneItem);

		final MenuItem scaleThreeQtrItem = new MenuItem(menu, SWT.RADIO);
		scaleThreeQtrItem.setText("3/4x");
		scaleThreeQtrItem.setData(Scale.SCALE_75);
		scaleList.add(scaleThreeQtrItem);

		final MenuItem scalehalfItem = new MenuItem(menu, SWT.RADIO);
		scalehalfItem.setText("1/2x");
		scalehalfItem.setData(Scale.SCALE_50);
		scaleList.add(scalehalfItem);

		final MenuItem scaleOneQtrItem = new MenuItem(menu, SWT.RADIO);
		scaleOneQtrItem.setText("1/4x");
		scaleOneQtrItem.setData(Scale.SCALE_25);
		scaleList.add(scaleOneQtrItem);

		SelectionAdapter selectionAdapter = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				MenuItem item = (MenuItem) e.getSource();

				boolean selection = item.getSelection();

				if (!selection) {
					return;
				}

				final int scale = ((Scale) item.getData()).value();

				shell.getDisplay().syncExec(new Runnable() {
					@Override
					public void run() {
						skinComposer.arrangeSkin(scale,
								currentState.getCurrentRotationId());

						/* location correction */
						Rectangle monitorBounds = Display.getDefault().getBounds();
						Rectangle emulatorBounds = shell.getBounds();
						shell.setLocation(
								Math.max(emulatorBounds.x, monitorBounds.x),
								Math.max(emulatorBounds.y, monitorBounds.y));
					}
				});

				DisplayStateData lcdStateData = new DisplayStateData(scale,
						currentState.getCurrentRotationId());
				communicator.sendToQEMU(SendCommand.SEND_DISPLAY_STATE,
						lcdStateData, false);

			}
		};

		for (MenuItem menuItem : scaleList) {
			int scale = ((Scale) menuItem.getData()).value();
			if (currentState.getCurrentScale() == scale) {
				menuItem.setSelection(true);
			}

			menuItem.addSelectionListener(selectionAdapter);
		}

		return menu;
	}

	public SelectionAdapter createKeyWindowMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				/* control the menu */
				if (getKeyWindowKeeper().isGeneralKeyWindow() == false) {
					MenuItem layoutSelected = (MenuItem) e.widget;

					if (layoutSelected.getSelection() == true) {
						for (MenuItem layout : layoutSelected.getParent()
								.getItems()) {
							if (layout != layoutSelected) {
								/* uncheck other menu items */
								layout.setSelection(false);
							} else {
								int layoutIndex = getKeyWindowKeeper()
										.getLayoutIndex();
								if (getKeyWindowKeeper().determineLayout() != layoutIndex) {
									/* switch */
									getKeyWindowKeeper().closeKeyWindow();
									layoutSelected.setSelection(true);
								}
							}
						}
					}
				}

				/* control the window */
				if (getKeyWindowKeeper().isSelectKeyWindowMenu() == true) { /* checked */
					if (getKeyWindowKeeper().getKeyWindow() == null) {
						if (getKeyWindowKeeper().getRecentlyDocked() != SWT.NONE) {
							getKeyWindowKeeper().openKeyWindow(
									getKeyWindowKeeper().getRecentlyDocked(),
									false);

							getKeyWindowKeeper().setRecentlyDocked(SWT.NONE);
						} else {
							/* opening for first time */
							getKeyWindowKeeper().openKeyWindow(
									SWT.RIGHT | SWT.CENTER, false);
						}
					} else {
						getKeyWindowKeeper().openKeyWindow(
								getKeyWindowKeeper().getDockPosition(), false);
					}
				} else { /* unchecked */
					if (getKeyWindowKeeper().getDockPosition() != SWT.NONE) {
						/* close the Key Window if it is docked to Main Window */
						getKeyWindowKeeper().setRecentlyDocked(
								getKeyWindowKeeper().getDockPosition());

						getKeyWindowKeeper().closeKeyWindow();
					} else {
						/* hide a Key Window */
						getKeyWindowKeeper().hideKeyWindow();
					}
				}
			}
		};

		return listener;
	};

	public SelectionAdapter createRamdumpMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Ram dump menu is selected");

				communicator.setRamdumpFlag(true);
				communicator.sendToQEMU(SendCommand.SEND_RAM_DUMP, null, false);

				RamdumpDialog ramdumpDialog;
				try {
					ramdumpDialog = new RamdumpDialog(shell, communicator,
							config);
					ramdumpDialog.open();
				} catch (IOException ee) {
					logger.log(Level.SEVERE, ee.getMessage(), ee);
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createScreenshotMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("ScreenShot Menu is selected");

				openScreenShotWindow();
			}
		};

		return listener;
	}

	public SelectionAdapter createHostKeyboardMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!communicator.isSensorDaemonStarted()) {
					SkinUtil.openMessage(shell, null,
							"Host Keyboard is not ready.\n"
									+ "Please wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					popupMenu.kbdOnItem.setSelection(isOnKbd);
					popupMenu.kbdOffItem.setSelection(!isOnKbd);

					return;
				}

				MenuItem item = (MenuItem) e.getSource();
				if (item.getSelection()) {
					boolean on = item.equals(popupMenu.kbdOnItem);
					isOnKbd = on;
					logger.info("Host Keyboard " + isOnKbd);

					communicator.sendToQEMU(SendCommand.SEND_HOST_KBD_STATE,
							new BooleanData(on, SendCommand.SEND_HOST_KBD_STATE.toString()), false);
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createAboutMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Open the about dialog");

				AboutDialog dialog = new AboutDialog(shell, config, imageRegistry);
				dialog.open();
			}
		};

		return listener;
	}

	public SelectionAdapter createForceCloseMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Force close is selected");

				int answer = SkinUtil.openMessage(shell, null,
						"If you force stop an emulator, it may cause some problems.\n"
								+ "Are you sure you want to continue?",
						SWT.ICON_QUESTION | SWT.OK | SWT.CANCEL, config);

				if (answer == SWT.OK) {
					logger.info("force close!!!");

					if (communicator != null) {
						communicator.terminate();
					}

					shell.getDisplay().asyncExec(new Runnable() {
						@Override
						public void run() {
							System.exit(-1);
						}
					});
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createShellMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (communicator.isSdbDaemonStarted() == false) {
					SkinUtil.openMessage(shell, null,
							"SDB is not ready.\n"
									+ "Please wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				String sdbPath = SkinUtil.getSdbPath();

				File sdbFile = new File(sdbPath);
				if (sdbFile.exists() == false) {
					logger.info("SDB file does not exist : " + sdbFile.getAbsolutePath());

					try {
						SkinUtil.openMessage(shell, null,
								"SDB file does not exist in the following path.\n"
										+ sdbFile.getCanonicalPath(),
								SWT.ICON_ERROR, config);
					} catch (IOException ee) {
						logger.log(Level.SEVERE, ee.getMessage(), ee);
					}

					return;
				}

				int portSdb = config.getArgInt(ArgsConstants.VM_BASE_PORT) + 1;

				ProcessBuilder procSdb = new ProcessBuilder();

				if (SwtUtil.isWindowsPlatform()) {
					procSdb.command("cmd.exe", "/c", "start", sdbPath, "sdb",
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isMacPlatform()) {
					procSdb.command("./sdbscript", "emulator-" + portSdb);
					/* procSdb.command( "/usr/X11/bin/uxterm", "-T",
							"emulator-" + portSdb, "-e", sdbPath,"shell"); */
				} else { /* Linux */
					procSdb.command("/usr/bin/gnome-terminal",
							"--disable-factory",
							"--title=" + SkinUtil.makeEmulatorName(config),
							"-x", sdbPath, "-s", "emulator-" + portSdb, "shell");
				}

				logger.info(procSdb.command().toString());

				try {
					procSdb.start(); /* open the sdb shell */
				} catch (Exception ee) {
					logger.log(Level.SEVERE, ee.getMessage(), ee);
					SkinUtil.openMessage(shell, null,
							"Fail to open Shell : \n" + ee.getMessage(),
							SWT.ICON_ERROR, config);
				}

				communicator.sendToQEMU(SendCommand.SEND_OPEN_SHELL, null, false);
			}
		};

		return listener;
	}

	public SelectionAdapter createEcpMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (communicator.isEcsServerStarted() == false) {
					SkinUtil.openMessage(shell, null,
							"Control Panel is not ready.\n"
							+ "Please wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				String ecpPath = SkinUtil.getEcpPath();

				File ecpFile = new File(ecpPath);
				if (ecpFile.exists() == false) {
					logger.info("Control Panel file does not exist : "
							+ ecpFile.getAbsolutePath());

					try {
						SkinUtil.openMessage(shell, null,
								"Control Panel file does not exist in the following path.\n"
								+ ecpFile.getCanonicalPath(),
								SWT.ICON_ERROR, config);
					} catch (IOException ee) {
						logger.log(Level.SEVERE, ee.getMessage(), ee);
					}

					return;
				}

				// TODO: thread
				/* get ECS port from Qemu */
				DataTranfer dataTranfer = communicator.sendDataToQEMU(
						SendCommand.SEND_ECP_PORT_REQ, null, true);
				byte[] receivedData = communicator.getReceivedData(dataTranfer);

				if (null != receivedData) {
					int portEcp = (receivedData[0] & 0xFF) << 24;
					portEcp |= (receivedData[1] & 0xFF) << 16;
					portEcp |= (receivedData[2] & 0xFF) << 8;
					portEcp |= (receivedData[3] & 0xFF);

					if (portEcp <= 0) {
						logger.log(Level.INFO, "ECS port failed : " + portEcp);

						SkinUtil.openMessage(shell, null,
								"Failed to connect to Control Server. Please restart the emulator.",
								SWT.ICON_ERROR, config);
						return;
					}

					String emulName = SkinUtil.getVmName(config);
					int portSdb = config.getArgInt(ArgsConstants.VM_BASE_PORT);

					ProcessBuilder procEcp = new ProcessBuilder();

					// FIXME: appropriate running binary setting is necessary.
					if (SwtUtil.isWindowsPlatform()) {
						procEcp.command("java.exe", "-jar", ecpPath,
								"vmname=" + emulName, "sdb.port=" + portSdb,
								"svr.port=" + portEcp);
					} else if (SwtUtil.isMacPlatform()) {
						procEcp.command("java", "-jar", "-XstartOnFirstThread", ecpPath,
								"vmname=" + emulName, "sdb.port=" + portSdb,
								"svr.port=" + portEcp);
					} else { /* Linux */
						procEcp.command("java", "-jar", ecpPath,
								"vmname=" + emulName, "sdb.port=" + portSdb,
								"svr.port=" + portEcp);
					}

					logger.info(procEcp.command().toString());

					try {
						procEcp.start(); /* open ECP */
					} catch (Exception ee) {
						logger.log(Level.SEVERE, ee.getMessage(), ee);
						SkinUtil.openMessage(shell, null,
								"Fail to open control panel : \n" + ee.getMessage(),
								SWT.ICON_ERROR, config);
					}
				} else {
					logger.severe("Fail to get ECP data");
					SkinUtil.openMessage(shell, null,
							"Fail to get ECP data", SWT.ICON_ERROR, config);
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createCloseMenuListener() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Close Menu is selected");

				communicator.sendToQEMU(SendCommand.SEND_CLOSE_REQ, null, false);
			}
		};

		return listener;
	}

	public void shutdown() {
		logger.info("shutdown the skin process");

		isShutdownRequested = true;

		if (!shell.isDisposed()) {
			shell.getDisplay().asyncExec(new Runnable() {
				@Override
				public void run() {
					if (!shell.isDisposed()) {
						shell.close();
					}
				}
			});
		}

	}

	public void keyForceRelease(boolean isMetaFilter) {
		if (isShutdownRequested == true) {
			return;
		}

		/* key event compensation */
		if (pressedKeyEventList.isEmpty() == false) {
			for (KeyEventData data : pressedKeyEventList) {
				if (isMetaFilter == true) {
					if (isMetaKey(data.keycode) == true) {
						/* keep multi-touching */
						continue;
					}
				}

				KeyEventData keyEventData = new KeyEventData(
						KeyEventType.RELEASED.value(), data.keycode,
						data.stateMask, data.keyLocation);
				communicator.sendToQEMU(SendCommand.SEND_KEYBOARD_KEY_EVENT,
						keyEventData, false);

				logger.info("auto release : keycode=" + keyEventData.keycode
						+ ", stateMask=" + keyEventData.stateMask
						+ ", keyLocation=" + keyEventData.keyLocation);
			}
		}

		pressedKeyEventList.clear();
	}
}
