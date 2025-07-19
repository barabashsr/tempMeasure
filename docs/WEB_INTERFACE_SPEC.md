# Web Interface Design Specification

## Overview
The web interface should provide a modern, responsive, and intuitive control panel for the Temperature Controller system. All interfaces must support both English and Russian languages.

## Design Principles
- **Consistency**: Unified look and feel across all pages
- **Responsiveness**: Works on desktop, tablet, and mobile
- **Performance**: Fast loading, minimal resource usage
- **Accessibility**: WCAG 2.1 AA compliant
- **Modern**: Clean, professional appearance

## Visual Design

### Color Palette
```css
:root {
  /* Primary Colors */
  --primary: #2196F3;        /* Blue - Main brand color */
  --primary-dark: #1976D2;   /* Darker blue for hover */
  --primary-light: #BBDEFB;  /* Light blue for backgrounds */
  
  /* Status Colors */
  --success: #4CAF50;        /* Green - Normal state */
  --warning: #FF9800;        /* Orange - Warning state */
  --danger: #F44336;         /* Red - Alarm/Error state */
  --info: #00BCD4;          /* Cyan - Information */
  
  /* Neutral Colors */
  --text-primary: #212121;   /* Main text */
  --text-secondary: #757575; /* Secondary text */
  --border: #E0E0E0;        /* Borders */
  --background: #FAFAFA;     /* Page background */
  --surface: #FFFFFF;        /* Card/Panel background */
  
  /* Dark Mode */
  --dark-background: #121212;
  --dark-surface: #1E1E1E;
  --dark-text: #FFFFFF;
}
```

### Typography
```css
/* Headings */
h1 { font-size: 2.5rem; font-weight: 300; }
h2 { font-size: 2rem; font-weight: 400; }
h3 { font-size: 1.5rem; font-weight: 400; }

/* Body */
body { 
  font-family: 'Roboto', -apple-system, BlinkMacSystemFont, sans-serif;
  font-size: 16px;
  line-height: 1.5;
}

/* Monospace for values */
.value { font-family: 'Roboto Mono', monospace; }
```

## Page Structure

### Common Header
```html
<header class="main-header">
  <div class="logo">
    <img src="/logo.svg" alt="Temperature Controller">
    <span class="system-name">Temperature Controller</span>
  </div>
  
  <nav class="main-nav">
    <a href="/" class="nav-link active">Dashboard</a>
    <div class="nav-dropdown">
      <a href="#" class="nav-link">Alarms <i class="fa fa-chevron-down"></i></a>
      <div class="dropdown-content">
        <a href="/alarms">Active Alarms</a>
        <a href="/alarm-config">Configuration</a>
        <a href="/alarm-history">History</a>
      </div>
    </div>
    <a href="/points" class="nav-link">Points</a>
    <div class="nav-dropdown">
      <a href="#" class="nav-link">System <i class="fa fa-chevron-down"></i></a>
      <div class="dropdown-content">
        <a href="/settings">General Settings</a>
        <a href="/network">Network</a>
        <a href="/time">Time Settings</a>
        <a href="/modbus">Modbus</a>
      </div>
    </div>
    <a href="/logs" class="nav-link">Logs</a>
  </nav>
  
  <div class="header-actions">
    <div class="language-toggle">
      <button class="lang-btn active" data-lang="en">EN</button>
      <button class="lang-btn" data-lang="ru">RU</button>
    </div>
    <div class="connection-status">
      <i class="fa fa-wifi"></i>
      <span class="status-text">Connected</span>
    </div>
  </div>
</header>
```

### Common Footer
```html
<footer class="main-footer">
  <div class="footer-info">
    <span>Device ID: <span class="device-id">1000</span></span>
    <span>Firmware: <span class="firmware-version">2.0</span></span>
    <span>Uptime: <span class="uptime">5d 14h 23m</span></span>
  </div>
  <div class="footer-actions">
    <button class="btn-icon" title="Download Logs">
      <i class="fa fa-download"></i>
    </button>
    <button class="btn-icon" title="System Info">
      <i class="fa fa-info-circle"></i>
    </button>
  </div>
</footer>
```

## Page Specifications

### 1. Dashboard Page (`/`)

#### Layout
```
+----------------------------------+
| Active Alarms Summary (if any)   |
+----------------------------------+
| Measurement Points Table         |
| Point | Name | Temp | Status |▼  |
|-------|------|------|---------|  |
| 01    | Room | 23.5 | Normal  |  |
| 02    | Hall | 24.1 | Normal  |  |
+----------------------------------+
| System Status Cards              |
| [Sensors] [Alarms] [Network]     |
+----------------------------------+
```

#### Features
- Auto-refresh every 5 seconds
- Click on temperature to show trend modal
- Color-coded rows based on status
- Sortable columns
- Search/filter functionality

#### Temperature Trend Modal
```javascript
// Modal structure
<div class="modal" id="trendModal">
  <div class="modal-content">
    <div class="modal-header">
      <h3>Temperature Trend - <span class="point-name"></span></h3>
      <button class="close-btn">&times;</button>
    </div>
    <div class="modal-body">
      <canvas id="trendChart"></canvas>
      <div class="chart-controls">
        <select class="time-range">
          <option value="1h">Last Hour</option>
          <option value="6h">Last 6 Hours</option>
          <option value="24h" selected>Last 24 Hours</option>
          <option value="7d">Last 7 Days</option>
        </select>
      </div>
    </div>
  </div>
</div>
```

### 2. Alarm Configuration Page (`/alarm-config`)

#### Layout
```
+----------------------------------+
| Alarm Configuration              |
+----------------------------------+
| Global Settings                  |
| [Hysteresis: 2.0°C] [Save]       |
+----------------------------------+
| Point Configuration Table        |
| Point | Name | Low Th | Low Pri | High Th | High Pri | Error Pri |
|-------|------|--------|---------|---------|----------|-----------|
| 01    | Room | -10.0  | Medium  | 30.0    | High     | Critical  |
+----------------------------------+
| Relay Behavior Settings          |
| Priority | Active | Acknowledged |
|----------|--------|--------------|
| Critical | S+B ON | Beacon ON    |
+----------------------------------+
```

#### Features
- Inline editing with validation
- Bulk operations (set all to priority)
- Import/Export CSV
- Real-time validation
- Undo/Redo capability

### 3. Active Alarms Page (`/alarms`)

#### Layout
```
+----------------------------------+
| Active Alarms (12)               |
+----------------------------------+
| Filter: [All Priorities] [All Types] [Search...]
+----------------------------------+
| Time | Point | Type | Value | Priority | Status | Actions |
|------|-------|------|-------|---------|--------|---------|
| 14:23| Pt 05 | High | 45.2°C| Critical| Active | [Ack]   |
+----------------------------------+
| Acknowledged Alarms (3)          |
+----------------------------------+
```

#### Features
- Real-time updates via WebSocket
- Bulk acknowledge
- Export to CSV
- Sound notification for new alarms
- Desktop notifications (optional)

### 4. Points Configuration Page (`/points`)

#### Layout
```
+----------------------------------+
| Measurement Points Configuration |
+----------------------------------+
| Point | Name | Sensor | Type | Actions |
|-------|------|--------|------|---------|
| 01    | Room | 28:AA..| DS18 | [Edit]  |
| 02    | Hall | <none> | -    | [Bind]  |
+----------------------------------+
| Available Sensors                |
+----------------------------------+
| ROM/Bus | Type | Temp | Actions |
|---------|------|------|---------|
| 28:FF...| DS18 | 23.1 | [Bind]  |
+----------------------------------+
```

#### Sensor Binding Modal
```html
<div class="modal" id="bindingModal">
  <h3>Bind Sensor to Point</h3>
  <form>
    <div class="form-group">
      <label>Select Point:</label>
      <select class="point-select">
        <option value="">-- Select Point --</option>
        <optgroup label="DS18B20 Points (0-49)">
          <option value="0">Point 00: Room A</option>
          <option value="1">Point 01: Room B</option>
        </optgroup>
        <optgroup label="PT1000 Points (50-59)">
          <option value="50">Point 50: Boiler</option>
        </optgroup>
      </select>
    </div>
    <div class="form-group">
      <label>Point Name:</label>
      <input type="text" class="point-name" maxlength="20">
    </div>
    <div class="form-actions">
      <button type="submit" class="btn-primary">Bind</button>
      <button type="button" class="btn-secondary">Cancel</button>
    </div>
  </form>
</div>
```

### 5. System Settings Pages

#### General Settings (`/settings`)
```
Device Settings:
- Device ID: [____] 
- Measurement Period: [__] seconds
- Display Timeout: [__] seconds

Alarm Settings:
- Critical Delay: [___] seconds
- High Delay: [___] seconds
- Medium Delay: [___] seconds
- Low Delay: [___] seconds

Relay Settings:
- Beacon On Time: [__] seconds
- Beacon Off Time: [__] seconds
```

#### Network Settings (`/network`)
```
WiFi Configuration:
- SSID: [____________]
- Password: [________]
- Hostname: [________]

Current Status:
- IP Address: 192.168.1.100
- MAC: AA:BB:CC:DD:EE:FF
- Signal: -45 dBm
```

#### Time Settings (`/time`)
```
Time Configuration:
- Current Time: 2024-12-30 14:23:45
- Timezone: [UTC+3:00 Moscow]
- NTP Server: [pool.ntp.org]
- [Sync Now]

RTC Status: Valid
Last Sync: 2024-12-30 12:00:00
```

### 6. Log Pages

#### Event Logs (`/logs`)
```
+----------------------------------+
| Event Logs                       |
+----------------------------------+
| Date Range: [Today] [Filter]     |
+----------------------------------+
| Time | Category | Message        |
|------|----------|----------------|
| 14:23| ALARM    | High temp Pt05 |
| 14:20| SYSTEM   | Config updated |
+----------------------------------+
```

## JavaScript Architecture

### API Service
```javascript
class APIService {
  constructor(baseURL = '/api') {
    this.baseURL = baseURL;
  }
  
  async getPoints() {
    return this.fetch('/points');
  }
  
  async updatePoint(id, data) {
    return this.fetch(`/points/${id}`, {
      method: 'PUT',
      body: JSON.stringify(data)
    });
  }
  
  async getAlarms() {
    return this.fetch('/alarms');
  }
  
  // ... other methods
  
  async fetch(endpoint, options = {}) {
    const response = await fetch(this.baseURL + endpoint, {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      },
      ...options
    });
    
    if (!response.ok) {
      throw new Error(`API Error: ${response.statusText}`);
    }
    
    return response.json();
  }
}
```

### Internationalization
```javascript
class I18n {
  constructor() {
    this.currentLang = localStorage.getItem('language') || 'en';
    this.translations = {};
  }
  
  async load(lang) {
    const response = await fetch(`/i18n/${lang}.json`);
    this.translations[lang] = await response.json();
    this.currentLang = lang;
    this.updateUI();
  }
  
  t(key) {
    const keys = key.split('.');
    let value = this.translations[this.currentLang];
    
    for (const k of keys) {
      value = value?.[k];
    }
    
    return value || key;
  }
  
  updateUI() {
    document.querySelectorAll('[data-i18n]').forEach(element => {
      const key = element.getAttribute('data-i18n');
      element.textContent = this.t(key);
    });
  }
}
```

## Translation Files

### English (`/i18n/en.json`)
```json
{
  "common": {
    "save": "Save",
    "cancel": "Cancel",
    "edit": "Edit",
    "delete": "Delete",
    "confirm": "Confirm",
    "search": "Search",
    "filter": "Filter"
  },
  "dashboard": {
    "title": "Dashboard",
    "temperature": "Temperature",
    "status": "Status",
    "normal": "Normal",
    "alarm": "Alarm",
    "error": "Error"
  },
  "alarms": {
    "title": "Alarms",
    "active": "Active Alarms",
    "acknowledged": "Acknowledged",
    "priority": {
      "critical": "Critical",
      "high": "High",
      "medium": "Medium",
      "low": "Low"
    },
    "type": {
      "high_temp": "High Temperature",
      "low_temp": "Low Temperature",
      "sensor_error": "Sensor Error"
    }
  }
}
```

### Russian (`/i18n/ru.json`)
```json
{
  "common": {
    "save": "Сохранить",
    "cancel": "Отмена",
    "edit": "Редактировать",
    "delete": "Удалить",
    "confirm": "Подтвердить",
    "search": "Поиск",
    "filter": "Фильтр"
  },
  "dashboard": {
    "title": "Панель управления",
    "temperature": "Температура",
    "status": "Статус",
    "normal": "Норма",
    "alarm": "Тревога",
    "error": "Ошибка"
  },
  "alarms": {
    "title": "Тревоги",
    "active": "Активные тревоги",
    "acknowledged": "Подтвержденные",
    "priority": {
      "critical": "Критический",
      "high": "Высокий",
      "medium": "Средний",
      "low": "Низкий"
    },
    "type": {
      "high_temp": "Высокая температура",
      "low_temp": "Низкая температура",
      "sensor_error": "Ошибка датчика"
    }
  }
}
```

## Responsive Design

### Breakpoints
```css
/* Mobile First Approach */
/* Base styles for mobile */

/* Tablet */
@media (min-width: 768px) {
  .container { max-width: 750px; }
}

/* Desktop */
@media (min-width: 1024px) {
  .container { max-width: 960px; }
}

/* Large Desktop */
@media (min-width: 1280px) {
  .container { max-width: 1140px; }
}
```

### Mobile Navigation
```css
/* Hamburger menu for mobile */
@media (max-width: 767px) {
  .main-nav {
    position: fixed;
    top: 60px;
    left: -100%;
    width: 80%;
    height: calc(100vh - 60px);
    background: var(--surface);
    transition: left 0.3s;
  }
  
  .main-nav.active {
    left: 0;
  }
}
```

## Performance Optimization

### 1. Resource Loading
```html
<!-- Preload critical resources -->
<link rel="preload" href="/css/main.css" as="style">
<link rel="preload" href="/fonts/roboto-v20-latin-regular.woff2" as="font" crossorigin>

<!-- Lazy load non-critical CSS -->
<link rel="preload" href="/css/charts.css" as="style" onload="this.onload=null;this.rel='stylesheet'">
```

### 2. JavaScript Optimization
```javascript
// Use dynamic imports for large libraries
async function showChart() {
  const { Chart } = await import('./chart.min.js');
  // Use Chart
}

// Debounce search input
function debounce(func, wait) {
  let timeout;
  return function executedFunction(...args) {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  };
}
```

### 3. API Optimization
- Use pagination for large datasets
- Implement caching with ETags
- Compress responses with gzip
- Use WebSocket for real-time updates

## Security Considerations

### 1. Authentication
```javascript
// Basic auth header
const auth = btoa(`${username}:${password}`);
fetch('/api/points', {
  headers: {
    'Authorization': `Basic ${auth}`
  }
});
```

### 2. CSRF Protection
```html
<meta name="csrf-token" content="{{ csrf_token }}">
```

### 3. Input Validation
```javascript
// Client-side validation
function validateTemperature(value) {
  const temp = parseFloat(value);
  return !isNaN(temp) && temp >= -40 && temp <= 200;
}

// Sanitize user input
function sanitizeInput(input) {
  const div = document.createElement('div');
  div.textContent = input;
  return div.innerHTML;
}
```

## Accessibility

### 1. ARIA Labels
```html
<button aria-label="Acknowledge alarm" class="btn-ack">
  <i class="fa fa-check" aria-hidden="true"></i>
</button>
```

### 2. Keyboard Navigation
```javascript
// Trap focus in modal
function trapFocus(element) {
  const focusableElements = element.querySelectorAll(
    'a[href], button, textarea, input[type="text"], input[type="radio"], input[type="checkbox"], select'
  );
  const firstFocusable = focusableElements[0];
  const lastFocusable = focusableElements[focusableElements.length - 1];
  
  element.addEventListener('keydown', function(e) {
    if (e.key === 'Tab') {
      if (e.shiftKey) { // Shift + Tab
        if (document.activeElement === firstFocusable) {
          lastFocusable.focus();
          e.preventDefault();
        }
      } else { // Tab
        if (document.activeElement === lastFocusable) {
          firstFocusable.focus();
          e.preventDefault();
        }
      }
    }
  });
}
```

### 3. Screen Reader Support
```html
<!-- Live regions for dynamic content -->
<div role="status" aria-live="polite" aria-atomic="true">
  <span class="sr-only">Temperature updated: 23.5°C</span>
</div>
```

## Testing Requirements

### 1. Browser Support
- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+
- Mobile Safari (iOS 14+)
- Chrome Mobile (Android 8+)

### 2. Performance Targets
- First Contentful Paint: < 1.5s
- Time to Interactive: < 3.5s
- Lighthouse Score: > 90

### 3. Accessibility Testing
- WAVE tool: 0 errors
- axe DevTools: 0 violations
- Keyboard navigation: Full support
- Screen reader: Tested with NVDA/JAWS
