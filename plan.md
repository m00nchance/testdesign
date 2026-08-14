# Wallpaper Manager for Windows - Project Plan

## Project Overview

Recreate the Linux wallpaper manager functionality (using `awww`, `slurp`, and `grim`) for Windows OS. This project will provide a seamless wallpaper management experience with screenshot-based selection, region picking, and automated wallpaper downloading capabilities.

### Original Linux Tools Reference

From the [43PR/dotfiles](https://github.com/43PR/dotfiles) repository, the wallpaper management system uses:

| Tool | Purpose | Linux/Wayland Equivalent for Windows |
|------|---------|--------------------------------------|
| `awww` | Anime wallpaper downloader/fetcher | Custom Python scraper or API integration |
| `slurp` | Region selector for screen area selection | PowerShell + .NET or Python with tkinter/PyQt |
| `grim` | Screenshot utility | PowerShell `Add-Type` with GDI+ or Python PIL+mss |

---

## Architecture Design

### System Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Wallpaper Manager GUI                     │
│                      (Electron/Python)                       │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  Wallpaper    │   │   Region      │   │  Screenshot   │
│  Downloader   │   │   Selector    │   │   Engine      │
│  (awww-win)   │   │  (slurp-win)  │   │  (grim-win)   │
└───────────────┘   └───────────────┘   └───────────────┘
        │                     │                     │
        └─────────────────────┼─────────────────────┘
                              ▼
                    ┌─────────────────┐
                    │  Wallpaper DB   │
                    │  (SQLite/JSON)  │
                    └─────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  Auto-Apply   │   │  Multi-Monitor│   │  Schedule     │
│   Service     │   │    Support    │   │   System      │
└───────────────┘   └───────────────┘   └───────────────┘
```

---

## Technical Specifications

### Phase 1: Core Infrastructure (Weeks 1-2)

#### 1.1 Technology Stack Selection

**Option A: Python-Based (Recommended)**
- **GUI Framework**: PyQt6 or CustomTkinter for modern UI
- **Screenshot**: `mss` library (cross-platform, fast)
- **Region Selection**: Custom overlay with PyQt/tkinter
- **Wallpaper Setting**: `ctypes` with Windows SPI (SystemParametersInfoW)
- **Database**: SQLite via `sqlite3` module
- **HTTP Requests**: `requests` + `aiohttp` for async downloads

**Option B: Electron + Node.js**
- **Framework**: Electron with React/Vue
- **Native Modules**: `robotjs` for screenshots, `wallpaper` npm package
- **Pros**: Better UI flexibility
- **Cons**: Heavier resource usage

**Decision**: Python-based for performance and native Windows integration

#### 1.2 Project Structure

```
wallpaper-manager-win/
├── src/
│   ├── __init__.py
│   ├── main.py                 # Application entry point
│   ├── core/
│   │   ├── __init__.py
│   │   ├── wallpaper_engine.py # Wallpaper setting logic
│   │   ├── screenshot.py       # grim-win implementation
│   │   ├── region_selector.py  # slurp-win implementation
│   │   └── downloader.py       # awww-win implementation
│   ├── gui/
│   │   ├── __init__.py
│   │   ├── main_window.py
│   │   ├── region_overlay.py
│   │   ├── gallery_view.py
│   │   └── settings_dialog.py
│   ├── database/
│   │   ├── __init__.py
│   │   ├── models.py
│   │   └── repository.py
│   ├── services/
│   │   ├── __init__.py
│   │   ├── scheduler.py
│   │   ├── monitor_detector.py
│   │   └── auto_starter.py
│   └── utils/
│       ├── __init__.py
│       ├── config.py
│       ├── logger.py
│       └── image_processor.py
├── resources/
│   ├── icons/
│   ├── themes/
│   └── default_wallpapers/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── e2e/
├── docs/
│   ├── api/
│   ├── user_guide/
│   └── development/
├── scripts/
│   ├── build.ps1
│   ├── install.ps1
│   └── setup_dev.ps1
├── requirements.txt
├── pyproject.toml
├── README.md
└── LICENSE
```

---

### Phase 2: Component Implementation (Weeks 3-6)

#### 2.1 grim-win: Screenshot Engine

**Requirements:**
- Full screen capture
- Multi-monitor support
- Specific window capture
- Region-based capture (integration with slurp-win)
- High-quality output (PNG, JPEG, WebP)
- Async capture capability

**Implementation Details:**

```python
# src/core/screenshot.py
import mss
import mss.tools
from PIL import Image
from typing import Optional, Tuple, List
import io

class ScreenshotEngine:
    """
    Windows equivalent of grim - Screenshot capture engine
    """
    
    def __init__(self):
        self.sct = mss.mss()
        self.monitors = self.sct.monitors[1:]  # Exclude primary monitor info
        
    def capture_full(self, monitor_index: int = 0, 
                     output_path: Optional[str] = None,
                     format: str = 'png') -> bytes:
        """
        Capture full monitor screen
        
        Args:
            monitor_index: Monitor number (0 = primary, 1+ = secondary)
            output_path: Optional file path to save
            format: Image format (png, jpg, webp)
            
        Returns:
            Image bytes
        """
        monitor = self.monitors[monitor_index] if monitor_index < len(self.monitors) else self.sct.primary
        screenshot = self.sct.grab(monitor)
        
        img = Image.frombytes('RGB', screenshot.size, screenshot.bgra, 'raw', 'BGRX')
        
        if output_path:
            img.save(output_path, format=format.upper())
        
        buffer = io.BytesIO()
        img.save(buffer, format=format.upper())
        return buffer.getvalue()
    
    def capture_region(self, left: int, top: int, 
                       width: int, height: int,
                       output_path: Optional[str] = None,
                       format: str = 'png') -> bytes:
        """
        Capture specific screen region
        
        Args:
            left, top: Top-left corner coordinates
            width, height: Region dimensions
            output_path: Optional file path
            format: Image format
            
        Returns:
            Image bytes
        """
        monitor = {
            'left': left,
            'top': top,
            'width': width,
            'height': height
        }
        
        screenshot = self.sct.grab(monitor)
        img = Image.frombytes('RGB', screenshot.size, screenshot.bgra, 'raw', 'BGRX')
        
        if output_path:
            img.save(output_path, format=format.upper())
        
        buffer = io.BytesIO()
        img.save(buffer, format=format.upper())
        return buffer.getvalue()
    
    def capture_window(self, window_title: str,
                       output_path: Optional[str] = None,
                       format: str = 'png') -> bytes:
        """
        Capture specific window by title
        
        Uses Win32 API to find window and capture its region
        """
        import win32gui
        import win32con
        
        hwnd = win32gui.FindWindow(None, window_title)
        if not hwnd:
            raise ValueError(f"Window '{window_title}' not found")
        
        left, top, right, bottom = win32gui.GetWindowRect(hwnd)
        width = right - left
        height = bottom - top
        
        return self.capture_region(left, top, width, height, output_path, format)
    
    def list_monitors(self) -> List[dict]:
        """Return information about all connected monitors"""
        return [
            {
                'index': i,
                'left': m['left'],
                'top': m['top'],
                'width': m['width'],
                'height': m['height'],
                'is_primary': i == 0
            }
            for i, m in enumerate(self.monitors)
        ]
```

#### 2.2 slurp-win: Region Selector

**Requirements:**
- Full-screen overlay
- Click-and-drag region selection
- Visual feedback (highlighted rectangle)
- Coordinate display
- Multi-monitor awareness
- Keyboard shortcuts (Enter to confirm, Esc to cancel)

**Implementation Details:**

```python
# src/core/region_selector.py
import tkinter as tk
from typing import Optional, Tuple, Callable
from dataclasses import dataclass

@dataclass
class SelectedRegion:
    left: int
    top: int
    width: int
    height: int
    monitor_index: int

class RegionSelectorOverlay:
    """
    Windows equivalent of slurp - Interactive region selection tool
    """
    
    def __init__(self, on_select: Callable[[SelectedRegion], None],
                 on_cancel: Callable[[], None]):
        self.on_select = on_select
        self.on_cancel = on_cancel
        self.start_x = 0
        self.start_y = 0
        self.current_rect = None
        self.selection_made = False
        
    def show(self) -> Optional[SelectedRegion]:
        """
        Display full-screen overlay for region selection
        
        Returns:
            SelectedRegion if successful, None if cancelled
        """
        self.root = tk.Tk()
        self.root.attributes('-fullscreen', True)
        self.root.attributes('-topmost', True)
        self.root.attributes('-alpha', 0.3)
        self.root.configure(bg='gray')
        self.root.configure(cursor='crosshair')
        
        # Get all monitor bounds
        self.monitors = self._get_monitor_bounds()
        
        self.canvas = tk.Canvas(
            self.root,
            highlightthickness=0,
            cursor='crosshair'
        )
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        # Bind events
        self.canvas.bind('<ButtonPress-1>', self._on_press)
        self.canvas.bind('<B1-Motion>', self._on_drag)
        self.canvas.bind('<ButtonRelease-1>', self._on_release)
        self.root.bind('<Escape>', lambda e: self._cancel())
        self.root.bind('<Return>', lambda e: self._confirm())
        
        # Info label for coordinates
        self.info_label = tk.Label(
            self.root,
            text='',
            bg='black',
            fg='white',
            font=('Consolas', 10),
            padx=10,
            pady=5
        )
        self.info_label.place(x=10, y=10)
        
        self.result: Optional[SelectedRegion] = None
        self.root.mainloop()
        
        return self.result
    
    def _get_monitor_bounds(self) -> list:
        """Get boundaries of all monitors using ctypes"""
        import ctypes
        user32 = ctypes.windll.user32
        
        class MONITORINFO(ctypes.Structure):
            _fields_ = [('cbSize', ctypes.c_int),
                       ('rcMonitor', ctypes.c_long * 4),
                       ('rcWork', ctypes.c_long * 4),
                       ('dwFlags', ctypes.c_ulong)]
        
        monitors = []
        
        def callback(hmon, dc, rect, data):
            mi = MONITORINFO()
            mi.cbSize = ctypes.sizeof(MONITORINFO)
            user32.GetMonitorInfoW(hmon, ctypes.byref(mi))
            monitors.append({
                'left': mi.rcMonitor[0],
                'top': mi.rcMonitor[1],
                'right': mi.rcMonitor[2],
                'bottom': mi.rcMonitor[3]
            })
            return True
        
        MONITORENUMPROC = ctypes.WINFUNCTYPE(
            ctypes.c_bool,
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_long * 4),
            ctypes.c_void_p
        )
        
        callback_func = MONITORENUMPROC(callback)
        user32.EnumDisplayMonitors(None, None, callback_func, 0)
        
        return monitors
    
    def _on_press(self, event):
        self.start_x = event.x
        self.start_y = event.y
        if self.current_rect:
            self.canvas.delete(self.current_rect)
    
    def _on_drag(self, event):
        if self.current_rect:
            self.canvas.delete(self.current_rect)
        
        x1, y1 = min(self.start_x, event.x), min(self.start_y, event.y)
        x2, y2 = max(self.start_x, event.x), max(self.start_y, event.y)
        
        self.current_rect = self.canvas.create_rectangle(
            x1, y1, x2, y2,
            outline='#00ff00',
            width=2,
            fill='',
            stipple='gray50'
        )
        
        width = x2 - x1
        height = y2 - y1
        self.info_label.config(text=f'X: {x1} Y: {y1}  W: {width} H: {height}')
    
    def _on_release(self, event):
        self.selection_made = True
    
    def _confirm(self):
        if self.selection_made and self.current_rect:
            coords = self.canvas.coords(self.current_rect)
            self.result = SelectedRegion(
                left=int(coords[0]),
                top=int(coords[1]),
                width=int(coords[2] - coords[0]),
                height=int(coords[3] - coords[1]),
                monitor_index=self._get_monitor_at(coords[0], coords[1])
            )
        self.root.destroy()
    
    def _cancel(self):
        self.result = None
        self.root.destroy()
    
    def _get_monitor_at(self, x: int, y: int) -> int:
        for i, m in enumerate(self.monitors):
            if m['left'] <= x <= m['right'] and m['top'] <= y <= m['bottom']:
                return i
        return 0
```

#### 2.3 awww-win: Wallpaper Downloader

**Requirements:**
- Fetch wallpapers from multiple sources
- Support for anime wallpapers (original awww purpose)
- Category/tag filtering
- Resolution filtering
- Batch download capability
- Progress tracking
- Cache management

**Implementation Details:**

```python
# src/core/downloader.py
import aiohttp
import asyncio
from typing import List, Optional, Dict, Any
from dataclasses import dataclass
from pathlib import Path
import hashlib

@dataclass
class Wallpaper:
    id: str
    title: str
    url: str
    thumbnail_url: str
    width: int
    height: int
    source: str
    tags: List[str]
    download_path: Optional[Path] = None

class WallpaperDownloader:
    """
    Windows equivalent of awww - Wallpaper discovery and download engine
    """
    
    SUPPORTED_SOURCES = {
        'wallhaven': 'https://wallhaven.cc/api/v1/search',
        'anime_wallpapers': 'https://www.anime-wallpapers.com/',
        'unsplash': 'https://api.unsplash.com/photos',
        'custom': None  # User-defined URLs
    }
    
    def __init__(self, cache_dir: Optional[Path] = None,
                 api_keys: Optional[Dict[str, str]] = None):
        self.cache_dir = cache_dir or Path.home() / '.wallpaper_manager' / 'cache'
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self.api_keys = api_keys or {}
        self.session: Optional[aiohttp.ClientSession] = None
        
    async def __aenter__(self):
        self.session = aiohttp.ClientSession()
        return self
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        if self.session:
            await self.session.close()
    
    async def search(self, query: str,
                     source: str = 'wallhaven',
                     min_width: int = 1920,
                     min_height: int = 1080,
                     tags: Optional[List[str]] = None,
                     limit: int = 50) -> List[Wallpaper]:
        """
        Search for wallpapers from specified source
        
        Args:
            query: Search term
            source: Wallpaper source identifier
            min_width: Minimum width filter
            min_height: Minimum height filter
            tags: Additional tag filters
            limit: Maximum results
            
        Returns:
            List of Wallpaper objects
        """
        if source == 'wallhaven':
            return await self._search_wallhaven(query, min_width, min_height, tags, limit)
        elif source == 'unsplash':
            return await self._search_unsplash(query, min_width, min_height, limit)
        elif source == 'anime_wallpapers':
            return await self._search_anime_wallpapers(query, tags, limit)
        else:
            raise ValueError(f"Unsupported source: {source}")
    
    async def _search_wallhaven(self, query: str,
                                 min_width: int, min_height: int,
                                 tags: Optional[List[str]],
                                 limit: int) -> List[Wallpaper]:
        """Search Wallhaven.cc API"""
        params = {
            'q': query,
            'atleast': f'{min_width}x{min_height}',
            'sorting': 'relevance',
            'order': 'desc',
            'page': '1'
        }
        
        if self.api_keys.get('wallhaven'):
            params['apikey'] = self.api_keys['wallhaven']
        
        wallpapers = []
        
        async with self.session.get(
            self.SUPPORTED_SOURCES['wallhaven'],
            params=params
        ) as response:
            if response.status == 200:
                data = await response.json()
                for item in data.get('data', [])[:limit]:
                    wallpapers.append(Wallpaper(
                        id=item['id'],
                        title=item.get('title', 'Untitled'),
                        url=item['path'],
                        thumbnail_url=item['thumbs']['small'],
                        width=item['resolution'].split('x')[0],
                        height=item['resolution'].split('x')[1],
                        source='wallhaven',
                        tags=item.get('tags', [])
                    ))
        
        return wallpapers
    
    async def _search_unsplash(self, query: str,
                                min_width: int, min_height: int,
                                limit: int) -> List[Wallpaper]:
        """Search Unsplash API"""
        if not self.api_keys.get('unsplash'):
            raise ValueError("Unsplash API key required")
        
        params = {
            'query': query,
            'per_page': limit,
            'orientation': 'landscape'
        }
        
        headers = {
            'Authorization': f'Client-ID {self.api_keys["unsplash"]}'
        }
        
        wallpapers = []
        
        async with self.session.get(
            self.SUPPORTED_SOURCES['unsplash'],
            params=params,
            headers=headers
        ) as response:
            if response.status == 200:
                data = await response.json()
                for item in data[:limit]:
                    wallpapers.append(Wallpaper(
                        id=str(item['id']),
                        title=item.get('description', 'Untitled'),
                        url=item['urls']['full'],
                        thumbnail_url=item['urls']['thumb'],
                        width=item['width'],
                        height=item['height'],
                        source='unsplash',
                        tags=item.get('tags', {}).get('results', [])
                    ))
        
        return wallpapers
    
    async def download(self, wallpaper: Wallpaper,
                       save_path: Optional[Path] = None,
                       progress_callback: Optional[callable] = None) -> Path:
        """
        Download wallpaper to local storage
        
        Args:
            wallpaper: Wallpaper object to download
            save_path: Custom save location (default: cache directory)
            progress_callback: Callback for download progress
            
        Returns:
            Path to downloaded file
        """
        if save_path is None:
            # Generate unique filename
            file_hash = hashlib.md5(wallpaper.url.encode()).hexdigest()[:12]
            ext = Path(wallpaper.url).suffix or '.jpg'
            save_path = self.cache_dir / f"{file_hash}{ext}"
        
        if save_path.exists():
            wallpaper.download_path = save_path
            return save_path
        
        async with self.session.get(wallpaper.url) as response:
            response.raise_for_status()
            total_size = int(response.headers.get('content-length', 0))
            downloaded = 0
            
            with open(save_path, 'wb') as f:
                async for chunk in response.content.iter_chunked(8192):
                    f.write(chunk)
                    downloaded += len(chunk)
                    
                    if progress_callback and total_size > 0:
                        progress_callback(downloaded / total_size * 100)
        
        wallpaper.download_path = save_path
        return save_path
    
    async def download_batch(self, wallpapers: List[Wallpaper],
                              concurrent_limit: int = 5,
                              progress_callback: Optional[callable] = None) -> List[Path]:
        """Download multiple wallpapers concurrently"""
        semaphore = asyncio.Semaphore(concurrent_limit)
        
        async def limited_download(wp):
            async with semaphore:
                return await self.download(wp, progress_callback=progress_callback)
        
        tasks = [limited_download(wp) for wp in wallpapers]
        return await asyncio.gather(*tasks, return_exceptions=True)
```

---

### Phase 3: Wallpaper Engine & Integration (Weeks 7-8)

#### 3.1 Windows Wallpaper Setter

**Requirements:**
- Set wallpaper via Windows SPI
- Support for different styles (fill, fit, stretch, tile, center, span)
- Multi-monitor wallpaper configuration
- Per-monitor wallpaper support
- Background color for non-fitting images

**Implementation Details:**

```python
# src/core/wallpaper_engine.py
import ctypes
import ctypes.wintypes
from enum import IntEnum
from pathlib import Path
from typing import Optional, List, Union

class WallpaperStyle(IntEnum):
    FILL = 0      # Fill (maintains aspect ratio, may crop)
    FIT = 1       # Fit (maintains aspect ratio, may have borders)
    STRETCH = 2   # Stretch (distorts to fit)
    TILE = 3      # Tile (repeats image)
    CENTER = 4    # Center (no scaling)
    SPAN = 5      # Span (across all monitors)

class WallpaperEngine:
    """
    Windows wallpaper management using SystemParametersInfoW
    """
    
    SPI_SETDESKWALLPAPER = 0x0014
    SPIF_UPDATEINIFILE = 0x01
    SPIF_SENDCHANGE = 0x02
    
    def __init__(self):
        self.user32 = ctypes.windll.user32
        self._current_style = WallpaperStyle.FILL
    
    def set_wallpaper(self, image_path: Union[str, Path],
                      style: WallpaperStyle = WallpaperStyle.FILL,
                      apply: bool = True) -> bool:
        """
        Set desktop wallpaper
        
        Args:
            image_path: Path to image file
            style: Wallpaper display style
            apply: Immediately apply changes
            
        Returns:
            True if successful
        """
        image_path = str(Path(image_path).absolute())
        
        # Set style in registry
        self._set_wallpaper_style(style)
        
        # Set wallpaper
        result = self.user32.SystemParametersInfoW(
            self.SPI_SETDESKWALLPAPER,
            0,
            image_path,
            self.SPIF_UPDATEINIFILE | self.SPIF_SENDCHANGE if apply else 0
        )
        
        return result != 0
    
    def _set_wallpaper_style(self, style: WallpaperStyle):
        """Set wallpaper style via registry"""
        import winreg
        
        key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            r'Control Panel\Desktop',
            0,
            winreg.KEY_SET_VALUE
        )
        
        # WallpaperStyle: 0=Tile, 1=Center, 2=Stretch, 3=Fit, 4=Fill, 5=Span
        style_map = {
            WallpaperStyle.TILE: '0',
            WallpaperStyle.CENTER: '1',
            WallpaperStyle.STRETCH: '2',
            WallpaperStyle.FIT: '3',
            WallpaperStyle.FILL: '4',
            WallpaperStyle.SPAN: '5'
        }
        
        winreg.SetValueEx(key, 'WallpaperStyle', 0, winreg.REG_SZ, style_map[style])
        
        # TileWallpaper
        tile_value = '1' if style == WallpaperStyle.TILE else '0'
        winreg.SetValueEx(key, 'TileWallpaper', 0, winreg.REG_SZ, tile_value)
        
        winreg.CloseKey(key)
    
    def set_multi_monitor_wallpaper(self, 
                                     wallpaper_config: List[dict],
                                     apply: bool = True) -> bool:
        """
        Set different wallpapers for each monitor
        
        Args:
            wallpaper_config: List of dicts with 'monitor' and 'path' keys
            apply: Immediately apply changes
            
        Example:
            [
                {'monitor': 0, 'path': r'C:\wallpapers\wp1.jpg'},
                {'monitor': 1, 'path': r'C:\wallpapers\wp2.jpg'}
            ]
        """
        import winreg
        
        # Build semicolon-separated path string
        paths = []
        for config in sorted(wallpaper_config, key=lambda x: x['monitor']):
            paths.append(str(Path(config['path']).absolute()))
        
        wallpaper_string = ';'.join(paths)
        
        key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            r'Control Panel\Desktop',
            0,
            winreg.KEY_SET_VALUE
        )
        
        winreg.SetValueEx(key, 'Wallpaper', 0, winreg.REG_SZ, wallpaper_string)
        winreg.SetValueEx(key, 'WallpaperStyle', 0, winreg.REG_SZ, '5')  # Span
        
        winreg.CloseKey(key)
        
        if apply:
            self.user32.SystemParametersInfoW(
                self.SPI_SETDESKWALLPAPER,
                0,
                wallpaper_string,
                self.SPIF_UPDATEINIFILE | self.SPIF_SENDCHANGE
            )
        
        return True
    
    def get_current_wallpaper(self) -> Optional[str]:
        """Get current wallpaper path"""
        import winreg
        
        try:
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r'Control Panel\Desktop',
                0,
                winreg.KEY_READ
            )
            
            path, _ = winreg.QueryValueEx(key, 'Wallpaper')
            winreg.CloseKey(key)
            return path
        except FileNotFoundError:
            return None
    
    def get_current_style(self) -> WallpaperStyle:
        """Get current wallpaper style"""
        import winreg
        
        try:
            key = winreg.OpenKey(
                winreg.HKEY_CURRENT_USER,
                r'Control Panel\Desktop',
                0,
                winreg.KEY_READ
            )
            
            style_str, _ = winreg.QueryValueEx(key, 'WallpaperStyle')
            winreg.CloseKey(key)
            
            style_map = {
                '0': WallpaperStyle.TILE,
                '1': WallpaperStyle.CENTER,
                '2': WallpaperStyle.STRETCH,
                '3': WallpaperStyle.FIT,
                '4': WallpaperStyle.FILL,
                '5': WallpaperStyle.SPAN
            }
            
            return style_map.get(style_str, WallpaperStyle.FILL)
        except FileNotFoundError:
            return WallpaperStyle.FILL
```

---

### Phase 4: GUI Development (Weeks 9-12)

#### 4.1 Main Application Window

**Features:**
- Modern dark/light theme support
- Wallpaper gallery grid view
- Search and filter functionality
- Drag-and-drop wallpaper import
- Right-click context menu
- Preview panel
- Settings dialog

**Technology**: PyQt6 with QML for modern UI or CustomTkinter for simplicity

---

### Phase 5: Advanced Features (Weeks 13-16)

#### 5.1 Scheduler Service
- Time-based wallpaper rotation
- Event-based triggers (login, unlock, time of day)
- Random shuffle mode
- Playlist management

#### 5.2 Multi-Monitor Intelligence
- Detect monitor arrangement
- Span wallpapers across monitors
- Per-monitor independent wallpapers
- Dynamic monitor change handling

#### 5.3 System Integration
- Windows startup integration
- System tray icon
- Notification support
- Power state awareness (battery vs plugged in)

---

## Database Schema

```sql
-- wallpapers table
CREATE TABLE wallpapers (
    id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    file_path TEXT NOT NULL,
    thumbnail_path TEXT,
    source TEXT,
    source_id TEXT,
    width INTEGER,
    height INTEGER,
    file_size INTEGER,
    hash TEXT UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_used_at TIMESTAMP,
    use_count INTEGER DEFAULT 0,
    rating INTEGER DEFAULT 0,
    is_favorite BOOLEAN DEFAULT FALSE
);

-- tags table
CREATE TABLE tags (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL
);

-- wallpaper_tags junction table
CREATE TABLE wallpaper_tags (
    wallpaper_id TEXT,
    tag_id INTEGER,
    PRIMARY KEY (wallpaper_id, tag_id),
    FOREIGN KEY (wallpaper_id) REFERENCES wallpapers(id),
    FOREIGN KEY (tag_id) REFERENCES tags(id)
);

-- playlists table
CREATE TABLE playlists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    shuffle_enabled BOOLEAN DEFAULT FALSE,
    interval_minutes INTEGER DEFAULT 30
);

-- playlist_items table
CREATE TABLE playlist_items (
    playlist_id INTEGER,
    wallpaper_id TEXT,
    position INTEGER,
    PRIMARY KEY (playlist_id, wallpaper_id),
    FOREIGN KEY (playlist_id) REFERENCES playlists(id),
    FOREIGN KEY (wallpaper_id) REFERENCES wallpapers(id)
);

-- monitor_config table
CREATE TABLE monitor_config (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    monitor_index INTEGER,
    monitor_id TEXT,
    wallpaper_id TEXT,
    style INTEGER,
    background_color TEXT,
    FOREIGN KEY (wallpaper_id) REFERENCES wallpapers(id)
);

-- settings table
CREATE TABLE settings (
    key TEXT PRIMARY KEY,
    value TEXT,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

---

## API Configuration

Users will need to configure API keys for external services:

```json
{
  "api_keys": {
    "wallhaven": "YOUR_WALLHAVEN_API_KEY",
    "unsplash": "YOUR_UNSPLASH_ACCESS_KEY"
  },
  "sources": {
    "wallhaven": {
      "enabled": true,
      "categories": ["anime", "general"],
      "purity": ["sfw"]
    },
    "unsplash": {
      "enabled": true,
      "collections": []
    },
    "local": {
      "enabled": true,
      "folders": [
        "~/Pictures/Wallpapers",
        "~/Downloads"
      ]
    }
  }
}
```

---

## Testing Strategy

### Unit Tests
- Screenshot engine (capture functions)
- Region selector (coordinate calculations)
- Downloader (URL parsing, caching)
- Wallpaper engine (registry operations)

### Integration Tests
- Full workflow: select region → capture → set as wallpaper
- Multi-monitor scenarios
- API interactions with mock servers

### End-to-End Tests
- Complete user workflows
- Performance benchmarks
- Memory leak detection

---

## Deployment

### Distribution Options
1. **Standalone Executable**: PyInstaller or cx_Freeze
2. **MSI Installer**: WiX Toolset or Inno Setup
3. **Microsoft Store**: UWP packaging (optional)
4. **Chocolatey/Scoop**: Package manager support

### Build Script Example (PowerShell)

```powershell
# scripts/build.ps1
param(
    [string]$Version = "1.0.0",
    [switch]$IncludeTests
)

# Create virtual environment
python -m venv .venv
.\.venv\Scripts\Activate.ps1

# Install dependencies
pip install -r requirements.txt
pip install pyinstaller

# Run tests if requested
if ($IncludeTests) {
    pytest tests/
}

# Build executable
pyinstaller --name "WallpaperManager" `
            --windowed `
            --icon "resources/icons/app.ico" `
            --add-data "resources;resources" `
            --hidden-import "PIL" `
            --hidden-import "mss" `
            src/main.py

# Create installer
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" scripts/installer.iss
```

---

## Timeline Summary

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| 1. Core Infrastructure | 2 weeks | Project structure, dependencies, basic utilities |
| 2. Component Implementation | 4 weeks | grim-win, slurp-win, awww-win modules |
| 3. Wallpaper Engine | 2 weeks | Windows wallpaper setting, multi-monitor support |
| 4. GUI Development | 4 weeks | Full application UI, gallery, settings |
| 5. Advanced Features | 4 weeks | Scheduler, system integration, polish |
| **Total** | **16 weeks** | **Production-ready application** |

---

## Success Criteria

1. ✅ Functional equivalents of all three Linux tools (awww, slurp, grim)
2. ✅ Native Windows integration (no WSL dependency)
3. ✅ Multi-monitor support
4. ✅ Modern, intuitive GUI
5. ✅ Wallpaper discovery from multiple sources
6. ✅ Automated scheduling and rotation
7. ✅ Performance: < 2s for region selection + capture
8. ✅ Memory usage < 200MB during normal operation
9. ✅ Comprehensive documentation
10. ✅ Automated testing suite with >80% coverage

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Windows API changes | Medium | Use stable SPI functions, avoid undocumented APIs |
| API rate limits | Low | Implement caching, respect rate limits, allow user API keys |
| Multi-monitor complexity | Medium | Extensive testing on various configurations |
| Performance issues | Low | Profile early, optimize image processing pipeline |
| Antivirus false positives | Medium | Code signing, clear documentation, whitelist instructions |

---

## Future Enhancements

- AI-powered wallpaper recommendations
- Dynamic wallpapers (animated/video backgrounds)
- Cloud sync for wallpaper collections
- Plugin system for custom sources
- Remote control via mobile app
- Integration with Windows Spotlight
- HDR wallpaper support
- OLED-safe dark wallpapers mode
