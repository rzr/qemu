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
import org.tizen.emulator.skin.custom.ColorTag;
import org.tizen.emulator.skin.custom.CustomProgressBar;
import org.tizen.emulator.skin.custom.SkinWindow;
import org.tizen.emulator.skin.dbi.HoverType;
import org.tizen.emulator.skin.dbi.RgbType;
import org.tizen.emulator.skin.dbi.RotationType;
import org.tizen.emulator.skin.dialog.AboutDialog;
import org.tizen.emulator.skin.dialog.DetailInfoDialog;
import org.tizen.emulator.skin.dialog.RamdumpDialog;
import org.tizen.emulator.skin.image.ImageRegistry;
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

	public class SkinReopenPolicy {
		private EmulatorSkin reopenSkin;
		private boolean reopen;

		private SkinReopenPolicy(EmulatorSkin reopenSkin, boolean reopen) {
			this.reopenSkin = reopenSkin;
			this.reopen = reopen;
		}

		public EmulatorSkin getReopenSkin() {
			return reopenSkin;
		}

		public boolean isReopen() {
			return reopen;
		}
	}

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

	private boolean isDisplayDragging;
	private boolean isShutdownRequested;
	private boolean isAboutToReopen;
	public boolean isOnTop;
	public boolean isKeyWindow;
	public boolean isOnKbd;
	private PopupMenu popupMenu;

	public Color colorVM;
	private KeyWindowKeeper keyWindowKeeper;
	public ColorTag pairTag;
	public CustomProgressBar bootingProgress;
	public ScreenShotDialog screenShotDialog;

	public SocketCommunicator communicator;
	private ShellListener shellListener;
	private MenuDetectListener shellMenuDetectListener;

	private MouseMoveListener canvasMouseMoveListener;
	private MouseListener canvasMouseListener;
	private MouseWheelListener canvasMouseWheelListener;
	private KeyListener canvasKeyListener;
	private MenuDetectListener canvasMenuDetectListener;
	private FocusListener canvasFocusListener;
	private LinkedList<KeyEventData> pressedKeyEventList;

	private EmulatorSkin reopenSkin;

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
			SkinUtil.setTopMost(shell, true);
		}

		this.displayCanvasStyle = displayCanvasStyle;

		/* prepare for VM state management */
		this.currentState = new EmulatorSkinState();

		setColorVM(); /* generate a identity color */
		this.keyWindowKeeper = new KeyWindowKeeper(this);
	}

	public void setCommunicator(SocketCommunicator communicator) {
		this.communicator = communicator;
	}

	private void setColorVM() {
		int portNumber = config.getArgInt(ArgsConstants.NET_BASE_PORT) % 100;

		if (portNumber >= 26200) {
			int red = (int) (Math.random() * 256);
			int green = (int) (Math.random() * 256);
			int blue = (int) (Math.random() * 256);
			this.colorVM = new Color(shell.getDisplay(), new RGB(red, green, blue));
		} else {
			int vmIndex = (portNumber % 100) / 10;

			SkinBasicColor colors[] = SkinBasicColor.values();
			this.colorVM = new Color(shell.getDisplay(), colors[vmIndex].color());
		}
	}

	public long initLayout() {
		imageRegistry = ImageRegistry.getInstance();

		/* create and attach a popup menu */
		isOnKbd = false;
		popupMenu = new PopupMenu(config, this, shell, imageRegistry);

		getKeyWindowKeeper().determineLayout();

		/* build a skin layout */
		if (skinInfo.isGeneralPurposeSkin() == false) {
			skinComposer = new ProfileSpecificSkinComposer(config, this,
					shell, currentState, imageRegistry, communicator);

			((ProfileSpecificSkinComposer) skinComposer).addProfileSpecificListener(shell);
		} else { /* general purpose skin */
			skinComposer = new GeneralPurposeSkinComposer(config, this,
					shell, currentState, imageRegistry);

			((GeneralPurposeSkinComposer) skinComposer).addGeneralPurposeListener(shell);
		}

		lcdCanvas = skinComposer.compose(displayCanvasStyle);

		/* load a hover color */
		currentState.setHoverColor(loadHoverColor());

		/* added event handlers */
		addMainWindowListeners();
		addCanvasListeners();

		setFocus();

		return 0;
	}

//	private void readyToReopen( EmulatorSkin sourceSkin, boolean isOnTop ) {
//
//		logger.info( "Start Changing AlwaysOnTop status" );
//
//		sourceSkin.reopenSkin = new EmulatorSkin( sourceSkin.config, isOnTop );
//
//		sourceSkin.reopenSkin.lcdCanvas = sourceSkin.lcdCanvas;
//		Point previousLocation = sourceSkin.shell.getLocation();
//
//		sourceSkin.reopenSkin.composeInternal( sourceSkin.lcdCanvas, previousLocation.x, previousLocation.y,
//				sourceSkin.currentLcdWidth, sourceSkin.currentLcdHeight, sourceSkin.currentScale,
//				sourceSkin.currentRotationId, sourceSkin.isOnKbd );
//
//		sourceSkin.reopenSkin.windowHandleId = sourceSkin.windowHandleId;
//
//		sourceSkin.reopenSkin.communicator = sourceSkin.communicator;
//		sourceSkin.reopenSkin.communicator.resetSkin( reopenSkin );
//
//		sourceSkin.isAboutToReopen = true;
//		sourceSkin.isShutdownRequested = true;
//
//		if ( sourceSkin.isScreenShotOpened && ( null != sourceSkin.screenShotDialog ) ) {
//			sourceSkin.screenShotDialog.setReserveImage( true );
//			sourceSkin.screenShotDialog.setEmulatorSkin( reopenSkin );
//			reopenSkin.isScreenShotOpened = true;
//			reopenSkin.screenShotDialog = sourceSkin.screenShotDialog;
//			// see open() method to know next logic for screenshot dialog.
//		}
//
//		sourceSkin.lcdCanvas.setParent( reopenSkin.shell );
//		sourceSkin.shell.close();
//
//	}

	private Color loadHoverColor() {
		HoverType hover = config.getDbiContents().getHover();

		if (null != hover) {
			RgbType hoverRgb = hover.getColor();
			if (null != hoverRgb) {
				Long r = hoverRgb.getR();
				Long g = hoverRgb.getG();
				Long b = hoverRgb.getB();

				if (null != r && null != g && null != b) {
					Color hoverColor = new Color(shell.getDisplay(),
							new RGB(r.intValue(), g.intValue(), b.intValue()));

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

	public SkinReopenPolicy open() {
		if (null == this.communicator) {
			logger.severe("communicator is null.");
			return null;
		}

		Display display = this.shell.getDisplay();

		this.shell.open();

		// logic only for reopen case ///////
//		if ( isScreenShotOpened && ( null != screenShotDialog ) ) {
//			try {
//				screenShotDialog.setReserveImage( false );
//				screenShotDialog.open();
//			} finally {
//				isScreenShotOpened = false;
//			}
//		}
		// ///////////////////////////////////

		while (!shell.isDisposed()) {
			if (!display.readAndDispatch()) {
				display.sleep();
			}
		}

		return new SkinReopenPolicy(reopenSkin, isAboutToReopen);
	}

	protected void skinFinalize() {
		logger.info("skinFinalize");

		skinComposer.composerFinalize();
	}

	private void addMainWindowListeners() {
		shellListener = new ShellListener() {
			@Override
			public void shellClosed(ShellEvent event) {
				logger.info("Main Window is closed");

				if (isShutdownRequested) {
					removeShellListeners();
					removeCanvasListeners();

					if (!isAboutToReopen) {
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
						if (keyWindow != null &&
								keyWindow.getShell().isVisible() == true) {
							dockValue = getKeyWindowKeeper().getDockPosition();
						}
						config.setSkinProperty(
								SkinPropertiesConstants.KEYWINDOW_POSITION, dockValue);

						config.saveSkinProperties();

						/* close the Key Window */
						if (getKeyWindowKeeper() != null) {
							getKeyWindowKeeper().closeKeyWindow();
						}

						/* dispose the color */
						if (colorVM != null) {
							colorVM.dispose();
						}
					}

					if (currentState.getCurrentImage() != null) {
						currentState.getCurrentImage().dispose();
					}
					if (currentState.getCurrentKeyPressedImage() != null) {
						currentState.getCurrentKeyPressedImage().dispose();
					}

					if (currentState.getHoverColor() != null) {
						currentState.getHoverColor().dispose();
					}

					skinFinalize();

				} else {
					/* Skin have to be alive until
					 * receiving shutdown request from qemu */
					event.doit = false;

					if (null != communicator) {
						communicator.sendToQEMU(SendCommand.CLOSE, null, false);
					}
				}
			}

			@Override
			public void shellActivated(ShellEvent event) {
				logger.info("activate");

				if (isKeyWindow == true && getKeyWindowKeeper().getKeyWindow() != null) {
					if (isOnTop == false) {
						getKeyWindowKeeper().getKeyWindow().getShell().moveAbove(shell);

						if (getKeyWindowKeeper().getDockPosition() != SWT.NONE) {
							shell.moveAbove(
									getKeyWindowKeeper().getKeyWindow().getShell());
						}
					} else {
						if (getKeyWindowKeeper().getDockPosition() == SWT.NONE) {
							getKeyWindowKeeper().getKeyWindow().getShell().moveAbove(shell);
						}
					}
				}
			}

			@Override
			public void shellDeactivated(ShellEvent event) {
				//logger.info("deactivate");

				/* do nothing */
			}

			@Override
			public void shellIconified(ShellEvent event) {
				logger.info("iconified");

				/* synchronization of minimization */
				shell.getDisplay().asyncExec(new Runnable() {
					@Override
					public void run() {
						if (isKeyWindow == true && getKeyWindowKeeper().getKeyWindow() != null) {
							Shell keyWindowShell = getKeyWindowKeeper().getKeyWindow().getShell();

							if (keyWindowShell.getMinimized() == false) {
								keyWindowShell.setVisible(false);
								/* the tool style window is exposed
								when even it was minimized */
								keyWindowShell.setMinimized(true);
							}
						}
					}
				});
			}

			@Override
			public void shellDeiconified(ShellEvent event) {
				logger.info("deiconified");

				if (isKeyWindow == true && getKeyWindowKeeper().getKeyWindow() != null) {
					Shell keyWindowShell = getKeyWindowKeeper().getKeyWindow().getShell();

					if (keyWindowShell.getMinimized() == true) {
						keyWindowShell.setMinimized(false);
						keyWindowShell.setVisible(true);
					}
				}

				DisplayStateData lcdStateData = new DisplayStateData(
						currentState.getCurrentScale(), currentState.getCurrentRotationId());
				communicator.sendToQEMU(
						SendCommand.CHANGE_LCD_STATE, lcdStateData, false);
			}
		};

		shell.addShellListener(shellListener);

		/* menu */
		shellMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				Menu menu = popupMenu.getMenuRoot();

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

	private MouseMoveListener makeDisplayTouchMoveLitener() {
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

	private MouseMoveListener makeDisplayMouseMoveLitener() {
		MouseMoveListener listener = new MouseMoveListener() {
			@Override
			public void mouseMove(MouseEvent e) {
				logger.info("mouse move : " + e);
				//TODO:
			}
		};

		return listener;
	}

	private MouseListener makeDisplayTouchClickLitener() {
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
				}
				else if (2 == e.button) /* wheel button */
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

	private MouseListener makeDisplayMouseClickLitener() {
		MouseListener listener = new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e) {
				logger.info("mouse up : " + e);
				//TODO:
			}

			@Override
			public void mouseDown(MouseEvent e) {
				logger.info("mouse down : " + e);
				//TODO:
			}

			@Override
			public void mouseDoubleClick(MouseEvent e) {
				logger.info("mouse double click : " + e);
				//TODO:
			}
		};

		return listener;
	}

	private MouseWheelListener makeDisplayMouseWheelLitener() {
		MouseWheelListener listener = new MouseWheelListener() {
			@Override
			public void mouseScrolled(MouseEvent e) {
				int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
						currentState.getCurrentResolutionWidth(),
						currentState.getCurrentResolutionHeight(),
						currentState.getCurrentScale(), currentState.getCurrentAngle());
				logger.info("mousewheel in display" +
						" x:" + geometry[0] + " y:" + geometry[1] + " value:" + e.count);

				int eventType;
				if (e.count < 0) {
					eventType = MouseEventType.WHEELDOWN.value();
				} else {
					eventType = MouseEventType.WHEELUP.value();
				}

				MouseEventData mouseEventData = new MouseEventData(
						MouseButtonType.WHEEL.value(), eventType,
						e.x, e.y, geometry[0], geometry[1], e.count);

				communicator.sendToQEMU(
						SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
			}
		};

		return listener;
	}

	private void addCanvasListeners() {
		/* menu */
		canvasMenuDetectListener = new MenuDetectListener() {
			@Override
			public void menuDetected(MenuDetectEvent e) {
				Menu menu = popupMenu.getMenuRoot();
				keyForceRelease(true);

				if (menu != null && isDisplayDragging == false) {
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
				//logger.info("gain focus");

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
		if (config.getArgBoolean(
				ArgsConstants.INPUT_MOUSE, false) == true) {
			canvasMouseMoveListener = makeDisplayMouseMoveLitener();
		} else {
			canvasMouseMoveListener = makeDisplayTouchMoveLitener();
		}
		lcdCanvas.addMouseMoveListener(canvasMouseMoveListener);

		if (config.getArgBoolean(
				ArgsConstants.INPUT_MOUSE, false) == true) {
			canvasMouseListener = makeDisplayMouseClickLitener();
		} else {
			canvasMouseListener = makeDisplayTouchClickLitener();
		}
		lcdCanvas.addMouseListener(canvasMouseListener);

		canvasMouseWheelListener = makeDisplayMouseWheelLitener();
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
					logger.info("'" + e.character + "':" +
							e.keyCode + ":" + e.stateMask + ":" + e.keyLocation);
				} else if (logger.isLoggable(Level.FINE)) {
					logger.fine(e.toString());
				}

				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				previous = null;

				/* generate a disappeared key event in Windows */
				if (SwtUtil.isWindowsPlatform() && disappearEvent) {
					disappearEvent = false;
					if (isMetaKey(e.keyCode) && e.character != '\0') {
						logger.info("send disappear release : keycode=" + disappearKeycode +
								", stateMask=" + disappearStateMask +
								", keyLocation=" + disappearKeyLocation);

						KeyEventData keyEventData = new KeyEventData(
								KeyEventType.RELEASED.value(),
								disappearKeycode, disappearStateMask, disappearKeyLocation);
						communicator.sendToQEMU(
								SendCommand.SEND_KEY_EVENT, keyEventData, false);

						removePressedKeyFromList(keyEventData);

						disappearKeycode = 0;
						disappearStateMask = 0;
						disappearKeyLocation = 0;
					}
				}

				keyReleasedDelivery(keyCode, stateMask, e.keyLocation);
			}

			@Override
			public void keyPressed(KeyEvent e) {
				int keyCode = e.keyCode;
				int stateMask = e.stateMask;

				/* When the pressed key event is overwritten by next key event,
				 * the release event of previous key does not occur in Windows.
				 * So, we generate a release key event by ourselves
				 * that had been disappeared. */
				if (SwtUtil.isWindowsPlatform()) {
					if (null != previous) {
						if (previous.keyCode != e.keyCode) {

							if (isMetaKey(previous.keyCode)) {
								disappearEvent = true;
								disappearKeycode = keyCode;
								disappearStateMask = stateMask;
								disappearKeyLocation = e.keyLocation;
							} else {
								/* three or more keys were pressed
								at the same time */
								if (disappearEvent == true) {
									logger.info("replace the disappearEvent : " +
											disappearKeycode + "->" + keyCode);

									disappearKeycode = keyCode;
									disappearStateMask = stateMask;
									disappearKeyLocation = e.keyLocation;
								}

								int previousKeyCode = previous.keyCode;
								int previousStateMask = previous.stateMask;

								if (logger.isLoggable(Level.INFO)) {
									logger.info("send previous release : '" +
											previous.character + "':" + previous.keyCode + ":" +
											previous.stateMask + ":" + previous.keyLocation);
								} else if (logger.isLoggable(Level.FINE)) {
									logger.fine("send previous release :" + previous.toString());
								}

								KeyEventData keyEventData = new KeyEventData(KeyEventType.RELEASED.value(),
										previousKeyCode, previousStateMask, previous.keyLocation);
								communicator.sendToQEMU(
										SendCommand.SEND_KEY_EVENT, keyEventData, false);

								removePressedKeyFromList(keyEventData);
							}

						}
					}
				} /* end isWindowsPlatform */

				if (logger.isLoggable(Level.INFO)) {
					logger.info("'" + e.character + "':" +
							e.keyCode + ":" + e.stateMask + ":" + e.keyLocation);
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
				MouseButtonType.LEFT.value(), eventType,
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(
				SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
	}

	protected void mouseUpDelivery(MouseEvent e) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());
		logger.info("mouseUp in display" +
				" x:" + geometry[0] + " y:" + geometry[1]);

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), MouseEventType.RELEASE.value(),
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(
				SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
	}

	protected void mouseDownDelivery(MouseEvent e) {
		int[] geometry = SkinUtil.convertMouseGeometry(e.x, e.y,
				currentState.getCurrentResolutionWidth(),
				currentState.getCurrentResolutionHeight(),
				currentState.getCurrentScale(), currentState.getCurrentAngle());
		logger.info("mouseDown in display" +
				" x:" + geometry[0] + " y:" + geometry[1]);

		MouseEventData mouseEventData = new MouseEventData(
				MouseButtonType.LEFT.value(), MouseEventType.PRESS.value(),
				e.x, e.y, geometry[0], geometry[1], 0);

		communicator.sendToQEMU(
				SendCommand.SEND_MOUSE_EVENT, mouseEventData, false);
	}

	protected void keyReleasedDelivery(int keyCode, int stateMask, int keyLocation) {
		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.RELEASED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEY_EVENT, keyEventData, false);

		removePressedKeyFromList(keyEventData);
	}

	protected void keyPressedDelivery(int keyCode, int stateMask, int keyLocation) {
		KeyEventData keyEventData = new KeyEventData(
				KeyEventType.PRESSED.value(), keyCode, stateMask, keyLocation);
		communicator.sendToQEMU(
				SendCommand.SEND_KEY_EVENT, keyEventData, false);

		addPressedKeyToList(keyEventData);
	}

	private boolean isMetaKey(int keyCode) {
		if (SWT.CTRL == keyCode ||
				SWT.ALT == keyCode ||
				SWT.SHIFT == keyCode ||
				SWT.COMMAND == keyCode) {
			return true;
		}

		return false;
	}

	protected synchronized boolean addPressedKeyToList(KeyEventData pressData) {
		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == pressData.keycode &&
					//data.stateMask == pressData.stateMask &&
					data.keyLocation == pressData.keyLocation) {
				return false;
			}
		}

		pressedKeyEventList.add(pressData);
		return true;
	}

	protected synchronized boolean removePressedKeyFromList(KeyEventData releaseData) {

		for (KeyEventData data : pressedKeyEventList) {
			if (data.keycode == releaseData.keycode &&
					//data.stateMask == releaseData.stateMask &&
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

	public void updateDisplay() {
		/* abstract */
	}

	protected void openScreenShotWindow() {
		/* abstract */
	}

	public void dispalyBrightness(boolean on) {
		if (on == true) {
			displayOn();
		} else {
			displayOff();
		}
	}

	protected void displayOn() {
		/* abstract */
	}

	protected void displayOff() {
		/* abstract */
	}

	/* for popup menu */
	public SelectionAdapter createDetailInfoMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (logger.isLoggable(Level.FINE)) {
					logger.fine("Open detail info");
				}

				String emulatorName = SkinUtil.makeEmulatorName(config);
				DetailInfoDialog detailInfoDialog = new DetailInfoDialog(
						shell, emulatorName, communicator, config, skinInfo);
				detailInfoDialog.open();
			}
		};

		return listener;
	}

	public SelectionAdapter createEcpMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {

				String emulName = SkinUtil.getVmName(config);
				int portSdb = config.getArgInt(ArgsConstants.NET_BASE_PORT);
				int portEcp = 0;

				DataTranfer dataTranfer = communicator.sendDataToQEMU(
						SendCommand.ECP_PORT_REQ, null, true);
				byte[] receivedData = communicator.getReceivedData(dataTranfer);
				portEcp = receivedData[0] << 24;
				portEcp |= receivedData[1] << 16;
				portEcp |= receivedData[2] << 8;
				portEcp |= receivedData[3];

				ProcessBuilder procEcp = new ProcessBuilder();

				// FIXME: appropriate running binary setting is necessary.
				if (SwtUtil.isLinuxPlatform()) {
					procEcp.command("/usr/bin/java", "-jar",
							"./emulator-control-panel.jar", "vmname="
									+ emulName, "sdb.port=" + portSdb,
							"svr.port=" + portEcp);
				} else if (SwtUtil.isWindowsPlatform()) {
					procEcp.command("java.exe", "-jar",
							"emulator-control-panel.jar", "vmname=" + emulName,
							"sdb.port=" + portSdb, "svr.port=" + portEcp);
				} else if (SwtUtil.isMacPlatform()) {
					// procSdb.command("./sdbscript", "emulator-" + portSdb);
					/*
					 * procSdb.command( "/usr/X11/bin/uxterm", "-T", "emulator-"
					 * + portSdb, "-e", sdbPath,"shell");
					 */
				}

				logger.log(Level.INFO, procEcp.command().toString());

				try {
					procEcp.start(); /* open ECP */
				} catch (Exception ee) {
					logger.log(Level.SEVERE, ee.getMessage(), ee);
					SkinUtil.openMessage(shell, null,
							"Fail to open control panel: \n" + ee.getMessage(),
							SWT.ICON_ERROR, config);
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createTopMostMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				isOnTop = popupMenu.onTopItem.getSelection();

				logger.info("Select Always On Top : " + isOnTop);
				// readyToReopen(EmulatorSkin.this, isOnTop);

				if (SkinUtil.setTopMost(shell, isOnTop) == false) {
					logger.info("failed to Always On Top");

					popupMenu.onTopItem.setSelection(isOnTop = false);
				} else {
					if (getKeyWindowKeeper().getKeyWindow() != null) {
						SkinUtil.setTopMost(
								getKeyWindowKeeper().getKeyWindow().getShell(), isOnTop);
					}
				}
			}
		};

		return listener;
	}

	public Menu createRotateMenu() {
		Menu menu = new Menu(shell, SWT.DROP_DOWN);

		final List<MenuItem> rotationList = new ArrayList<MenuItem>();

		Iterator<Entry<Short, RotationType>> iterator = SkinRotation.getRotationIterator();

		while ( iterator.hasNext() ) {

			Entry<Short, RotationType> entry = iterator.next();
			Short rotationId = entry.getKey();
			RotationType section = entry.getValue();

			final MenuItem menuItem = new MenuItem( menu, SWT.RADIO );
			menuItem.setText( section.getName().value() );
			menuItem.setData( rotationId );

			if (currentState.getCurrentRotationId() == rotationId) {
				menuItem.setSelection( true );
			}

			rotationList.add( menuItem );

		}

		/* temp : swap rotation menu names */
		if (currentState.getCurrentResolutionWidth() >
				currentState.getCurrentResolutionHeight())
		{
			for (MenuItem m : rotationList) {
				short rotationId = (Short) m.getData();

				if (rotationId == RotationInfo.PORTRAIT.id()) {
					String landscape = SkinRotation.getRotation(
							RotationInfo.LANDSCAPE.id()).getName().value();
					m.setText(landscape);
				} else if (rotationId == RotationInfo.LANDSCAPE.id()) {
					String portrait = SkinRotation.getRotation(
							RotationInfo.PORTRAIT.id()).getName().value();
					m.setText(portrait);
				} else if (rotationId == RotationInfo.REVERSE_PORTRAIT.id()) {
					String landscapeReverse = SkinRotation.getRotation(
							RotationInfo.REVERSE_LANDSCAPE.id()).getName().value();
					m.setText(landscapeReverse);
				} else if (rotationId == RotationInfo.REVERSE_LANDSCAPE.id()) {
					String portraitReverse = SkinRotation.getRotation(
							RotationInfo.REVERSE_PORTRAIT.id()).getName().value();
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
							"Rotation is not ready.\n" +
							"Please wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				final short rotationId = ((Short) item.getData());

				shell.getDisplay().syncExec(new Runnable() {
					@Override
					public void run() {
						skinComposer.arrangeSkin(currentState.getCurrentScale(), rotationId);

						/* location correction */
						Rectangle monitorBounds = Display.getDefault().getBounds();
						Rectangle emulatorBounds = shell.getBounds();
						shell.setLocation(
								Math.max(emulatorBounds.x, monitorBounds.x),
								Math.max(emulatorBounds.y, monitorBounds.y));
					}
				});

				DisplayStateData lcdStateData =
						new DisplayStateData(currentState.getCurrentScale(), rotationId);
				communicator.sendToQEMU(
						SendCommand.CHANGE_LCD_STATE, lcdStateData, false);
			}
		};

		for (MenuItem menuItem : rotationList) {
			menuItem.addSelectionListener(selectionAdapter);
		}

		return menu;
	}

	public Menu createScaleMenu() {
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
						skinComposer.arrangeSkin(scale, currentState.getCurrentRotationId());

						/* location correction */
						Rectangle monitorBounds = Display.getDefault().getBounds();
						Rectangle emulatorBounds = shell.getBounds();
						shell.setLocation(
								Math.max(emulatorBounds.x, monitorBounds.x),
								Math.max(emulatorBounds.y, monitorBounds.y));
					}
				});

				DisplayStateData lcdStateData =
						new DisplayStateData(scale, currentState.getCurrentRotationId());
				communicator.sendToQEMU(
						SendCommand.CHANGE_LCD_STATE, lcdStateData, false);

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

	public SelectionAdapter createKeyWindowMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				/* control the menu */
				if (getKeyWindowKeeper().isGeneralKeyWindow() == false) {
					MenuItem layoutSelected = (MenuItem) e.widget;

					if (layoutSelected.getSelection() == true) {
						for (MenuItem layout : layoutSelected.getParent().getItems()) {
							if (layout != layoutSelected) {
								/* uncheck other menu items */
								layout.setSelection(false);
							} else {
								int layoutIndex = getKeyWindowKeeper().getLayoutIndex();
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
				if (getKeyWindowKeeper().isSelectKeyWindowMenu() == true)
				{ /* checked */
					if (getKeyWindowKeeper().getKeyWindow() == null) {
						if (getKeyWindowKeeper().getRecentlyDocked() != SWT.NONE) {
							getKeyWindowKeeper().openKeyWindow(
									getKeyWindowKeeper().getRecentlyDocked(), false);

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
				}
				else
				{ /* unchecked */
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

	public SelectionAdapter createRamdumpMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Ram dump menu is selected");

				communicator.setRamdumpFlag(true);
				communicator.sendToQEMU(
						SendCommand.RAM_DUMP, null, false);

				RamdumpDialog ramdumpDialog;
				try {
					ramdumpDialog = new RamdumpDialog(shell, communicator, config);
					ramdumpDialog.open();
				} catch (IOException ee) {
					logger.log( Level.SEVERE, ee.getMessage(), ee);
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createScreenshotMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("ScreenShot Menu is selected");

				openScreenShotWindow();
			}
		};

		return listener;
	}

	public SelectionAdapter createHostKeyboardMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!communicator.isSensorDaemonStarted()) {
					SkinUtil.openMessage(shell, null,
							"Host Keyboard is not ready.\n" +
							"Please wait until the emulator is completely boot up.",
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

					communicator.sendToQEMU(SendCommand.HOST_KBD,
							new BooleanData(on, SendCommand.HOST_KBD.toString()), false);
				}
			}
		};

		return listener;
	}

	public SelectionAdapter createAboutMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Open the about dialog");

				AboutDialog dialog = new AboutDialog(shell, config);
				dialog.open();
			}
		};

		return listener;
	}

	public SelectionAdapter createForceCloseMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Force close is selected");

				int answer = SkinUtil.openMessage(shell, null,
						"If you force stop an emulator, it may cause some problems.\n" +
						"Are you sure you want to contiue?",
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

	public SelectionAdapter createShellMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				if (!communicator.isSdbDaemonStarted()) {
					SkinUtil.openMessage(shell, null, "SDB is not ready.\n" +
							"Please wait until the emulator is completely boot up.",
							SWT.ICON_WARNING, config);
					return;
				}

				String sdbPath = SkinUtil.getSdbPath();

				File sdbFile = new File(sdbPath);
				if (!sdbFile.exists()) {
					logger.log(Level.INFO, "SDB file is not exist : " + sdbFile.getAbsolutePath());
					try {
						SkinUtil.openMessage(shell, null,
								"SDB file is not exist in the following path.\n" +
								sdbFile.getCanonicalPath(),
								SWT.ICON_ERROR, config);
					} catch (IOException ee) {
						logger.log(Level.SEVERE, ee.getMessage(), ee);
					}
					return;
				}

				int portSdb = config.getArgInt(ArgsConstants.NET_BASE_PORT);

				ProcessBuilder procSdb = new ProcessBuilder();

				if (SwtUtil.isLinuxPlatform()) {
					procSdb.command("/usr/bin/gnome-terminal", "--disable-factory",
							"--title=" + SkinUtil.makeEmulatorName( config ), "-x", sdbPath,
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isWindowsPlatform()) {
					procSdb.command("cmd.exe", "/c", "start", sdbPath, "sdb",
							"-s", "emulator-" + portSdb, "shell");
				} else if (SwtUtil.isMacPlatform()) {
					procSdb.command("./sdbscript", "emulator-" + portSdb);
					/* procSdb.command(
					"/usr/X11/bin/uxterm", "-T", "emulator-" + portSdb, "-e", sdbPath,"shell"); */
				}

				logger.log(Level.INFO, procSdb.command().toString());

				try {
					procSdb.start(); /* open the sdb shell */
				} catch (Exception ee) {
					logger.log(Level.SEVERE, ee.getMessage(), ee);
					SkinUtil.openMessage(shell, null, "Fail to open Shell: \n" +
							ee.getMessage(), SWT.ICON_ERROR, config);
				}

				communicator.sendToQEMU(
						SendCommand.OPEN_SHELL, null, false);
			}
		};

		return listener;
	}

	public SelectionAdapter createCloseMenu() {
		SelectionAdapter listener = new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e) {
				logger.info("Close Menu is selected");

				communicator.sendToQEMU(
						SendCommand.CLOSE, null, false);
			}
		};

		return listener;
	}

	public void shutdown() {
		logger.info("shutdown the skin process");

		isShutdownRequested = true;

		if (!this.shell.isDisposed()) {
			this.shell.getDisplay().asyncExec(new Runnable() {
				@Override
				public void run() {
					if (!shell.isDisposed()) {
						EmulatorSkin.this.shell.close();
					}
				}
			} );
		}

	}

	public void keyForceRelease(boolean isMetaFilter) {
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
				communicator.sendToQEMU(
						SendCommand.SEND_KEY_EVENT, keyEventData, false);

				logger.info("auto release : keycode=" + keyEventData.keycode +
						", stateMask=" + keyEventData.stateMask +
						", keyLocation=" + keyEventData.keyLocation);
			}
		}

		pressedKeyEventList.clear();
	}
}
