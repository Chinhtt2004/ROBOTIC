'use client';
import React, { useState, useEffect, useRef } from 'react';
import {
  Home, Settings, Wifi, Info, Shield, HelpCircle,
  Video, Battery, Activity, Zap, Thermometer, Map,
  ChevronUp, ChevronDown, ChevronLeft, ChevronRight,
  Sun, Volume2, Lightbulb, StopCircle, Maximize, PlaySquare, StopCircle as StopIcon
} from 'lucide-react';

export default function Dashboard() {
  const [ws, setWs] = useState(null);
  const [telemetry, setTelemetry] = useState({
    connected: false,
    battery: 85,
    speed: 0,
    voltage: 7.6,
    temperature: 25,
    distance: 12.4,
    runtime: '00:15:32',
  });

  const [activeMode, setActiveMode] = useState('Manual');
  const [speed, setSpeed] = useState(50);
  const [steeringTrim, setSteeringTrim] = useState(0);

  // Khởi tạo WebSocket kết nối với Backend Node.js
  useEffect(() => {
    const socket = new WebSocket('ws://192.168.50.254:8080');

    socket.onopen = () => {
      console.log('Connected to Backend');
      setTelemetry(prev => ({ ...prev, connected: true }));
      setWs(socket);
    };

    socket.onmessage = (event) => {
      try {
        console.log(event.data);
        const data = JSON.parse(event.data);
        if (data.type === 'telemetry') {
          setTelemetry(prev => ({ ...prev, ...data }));
        }
      } catch (err) { }
    };

    socket.onclose = () => {
      console.log('Disconnected');
      setTelemetry(prev => ({ ...prev, connected: false }));
      setWs(null);
    };

    return () => socket.close();
  }, []);

  // Hàm gửi lệnh điều khiển xe (đã tích hợp thanh trượt Speed)
  const sendDriveCommand = (x, y) => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      // Nhân với tỷ lệ phần trăm tốc độ (0 - 100%)
      const scale = speed / 100;
      const scaledX = Math.round(x * scale);
      const scaledY = Math.round(y * scale);
      ws.send(JSON.stringify({ type: 'drive', x: scaledX, y: scaledY }));
    }
  };

  // Bắt sự kiện bàn phím
  useEffect(() => {
    const handleKeyDown = (e) => {
      // Giả lập tọa độ Joystick: Tiến y=-512, Lùi y=511. 
      // Do cơ cấu mạch Freenove: Trục X bị ngược (Kéo sang phải thì điện áp giảm -> x âm)
      // Nên Trái x=511, Phải x=-512
      if (e.key === 'ArrowUp') sendDriveCommand(0, -512);
      if (e.key === 'ArrowDown') sendDriveCommand(0, 511);
      if (e.key === 'ArrowLeft') sendDriveCommand(511, 0);
      if (e.key === 'ArrowRight') sendDriveCommand(-512, 0);
    };

    const handleKeyUp = (e) => {
      sendDriveCommand(0, 0);
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [ws, speed]); // Thêm speed vào để luôn lấy tốc độ mới nhất

  return (
    <div className="flex h-screen bg-slate-50 text-slate-800 font-sans">

      {/* Sidebar */}
      <div className="w-20 bg-white border-r flex flex-col items-center py-6 gap-8 shadow-sm z-10">
        <div className="p-3 bg-blue-50 text-blue-600 rounded-xl cursor-pointer">
          <Home size={24} />
          <span className="text-[10px] block text-center mt-1 font-semibold">Home</span>
        </div>
        <div className="p-3 text-slate-400 hover:text-blue-600 cursor-pointer transition-colors">
          <Settings size={24} />
        </div>
        <div className="p-3 text-slate-400 hover:text-blue-600 cursor-pointer transition-colors">
          <Wifi size={24} />
        </div>
        <div className="p-3 text-slate-400 hover:text-blue-600 cursor-pointer transition-colors mt-auto">
          <Info size={24} />
        </div>
      </div>

      {/* Main Content */}
      <div className="flex-1 flex flex-col overflow-hidden">

        {/* Header Title */}
        <header className="px-8 py-6 bg-white border-b flex justify-between items-end">
          <div>
            <h1 className="text-2xl font-bold text-slate-800">Dashboard</h1>
            <p className="text-slate-500 text-sm mt-1">Real-time control and monitoring of your Freenove Smart Robot Car</p>
          </div>
          <div className="flex items-center gap-2">
            <span className={`w-3 h-3 rounded-full ${telemetry.connected ? 'bg-green-500' : 'bg-red-500'}`}></span>
            <span className="text-sm font-medium text-slate-600">{telemetry.connected ? 'Connected' : 'Disconnected'}</span>
          </div>
        </header>

        {/* Scrollable Content */}
        <div className="flex-1 overflow-y-auto p-8">
          <div className="grid grid-cols-12 gap-8 max-w-7xl mx-auto">

            {/* Left Column (Status & Controls) */}
            <div className="col-span-12 lg:col-span-4 flex flex-col gap-6">

              {/* Robot Status */}
              <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-100 relative overflow-hidden">
                <h2 className="text-sm font-semibold text-slate-500 uppercase tracking-wider mb-6 flex items-center gap-2">
                  <span className="w-2 h-2 rounded-full bg-green-500 block"></span> Robot Status
                </h2>

                <div className="flex justify-between items-center mb-6">
                  <div>
                    <p className="text-xs text-slate-400 mb-1">Mode</p>
                    <p className="font-semibold text-blue-600">{activeMode}</p>
                  </div>
                  <div>
                    <p className="text-xs text-slate-400 mb-1">Speed</p>
                    <p className="font-semibold">{telemetry.speed || 0} cm/s</p>
                  </div>
                  <div>
                    <p className="text-xs text-slate-400 mb-1">Battery</p>
                    <p className="font-semibold">{telemetry.battery_percent || 0}%</p>
                  </div>
                </div>

                <div className="mb-4">
                  <div className="w-full bg-slate-100 rounded-full h-1.5">
                    <div className="bg-green-500 h-1.5 rounded-full" style={{ width: `${telemetry.battery_percent || 0}%` }}></div>
                  </div>
                </div>

                <div>
                  <p className="text-xs text-slate-400 mb-1">IP Address</p>
                  <p className="font-mono text-sm text-slate-600">192.168.4.1</p>
                </div>
              </div>

              {/* Control Panel (D-Pad & Sliders) */}
              <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-100 flex-1">
                <h2 className="text-sm font-semibold text-slate-500 uppercase tracking-wider mb-6">Control Panel</h2>

                {/* D-Pad */}
                <div className="flex flex-col items-center justify-center gap-2 mb-10">
                  <button
                    onMouseDown={() => sendDriveCommand(0, -512)} onMouseUp={() => sendDriveCommand(0, 0)} onMouseLeave={() => sendDriveCommand(0, 0)}
                    className="w-14 h-14 bg-slate-100 hover:bg-blue-50 hover:text-blue-600 rounded-xl flex items-center justify-center transition-colors shadow-sm active:bg-blue-100">
                    <ChevronUp size={28} strokeWidth={2.5} />
                  </button>
                  <div className="flex gap-2">
                    <button
                      onMouseDown={() => sendDriveCommand(511, 0)} onMouseUp={() => sendDriveCommand(0, 0)} onMouseLeave={() => sendDriveCommand(0, 0)}
                      className="w-14 h-14 bg-slate-100 hover:bg-blue-50 hover:text-blue-600 rounded-xl flex items-center justify-center transition-colors shadow-sm active:bg-blue-100">
                      <ChevronLeft size={28} strokeWidth={2.5} />
                    </button>
                    <div className="w-14 h-14"></div> {/* Center empty */}
                    <button
                      onMouseDown={() => sendDriveCommand(-512, 0)} onMouseUp={() => sendDriveCommand(0, 0)} onMouseLeave={() => sendDriveCommand(0, 0)}
                      className="w-14 h-14 bg-slate-100 hover:bg-blue-50 hover:text-blue-600 rounded-xl flex items-center justify-center transition-colors shadow-sm active:bg-blue-100">
                      <ChevronRight size={28} strokeWidth={2.5} />
                    </button>
                  </div>
                  <button
                    onMouseDown={() => sendDriveCommand(0, 511)} onMouseUp={() => sendDriveCommand(0, 0)} onMouseLeave={() => sendDriveCommand(0, 0)}
                    className="w-14 h-14 bg-slate-100 hover:bg-blue-50 hover:text-blue-600 rounded-xl flex items-center justify-center transition-colors shadow-sm active:bg-blue-100">
                    <ChevronDown size={28} strokeWidth={2.5} />
                  </button>
                </div>

                {/* Sliders */}
                <div className="space-y-6">
                  <div>
                    <div className="flex justify-between text-xs font-semibold text-slate-500 mb-3">
                      <span>Speed</span>
                      <span className="text-blue-600">{speed}%</span>
                    </div>
                    <input type="range" min="0" max="100" value={speed} onChange={(e) => setSpeed(e.target.value)} className="w-full h-1.5 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-blue-600" />
                  </div>
                  <div>
                    <div className="flex justify-between text-xs font-semibold text-slate-500 mb-3">
                      <span>Steering Trim</span>
                      <span className="text-blue-600">{steeringTrim}</span>
                    </div>
                    <input type="range" min="-50" max="50" value={steeringTrim} onChange={(e) => setSteeringTrim(e.target.value)} className="w-full h-1.5 bg-slate-200 rounded-lg appearance-none cursor-pointer accent-blue-600" />
                  </div>
                </div>
              </div>
            </div>

            {/* Middle Column (Camera) */}
            <div className="col-span-12 lg:col-span-5 flex flex-col gap-6">
              {/* Camera Box */}
              <div className="bg-white rounded-2xl p-4 shadow-sm border border-slate-100 flex-1 flex flex-col relative">
                <div className="flex justify-between items-center mb-4">
                  <h2 className="text-sm font-semibold text-slate-500 uppercase tracking-wider">Camera Live Feed</h2>
                  <div className="flex items-center gap-3 text-slate-400">
                    <Maximize size={18} className="cursor-pointer hover:text-slate-700" />
                    <Video size={18} className="cursor-pointer hover:text-slate-700" />
                  </div>
                </div>

                <div className="flex-1 bg-slate-200 rounded-xl overflow-hidden relative min-h-[300px]">
                  {/* Placeholder for camera stream */}
                  <div className="absolute inset-0 flex items-center justify-center text-slate-400 flex-col gap-2">
                    <Video size={48} strokeWidth={1.5} />
                    <span className="text-sm">Connecting to stream...</span>
                  </div>
                  <div className="absolute top-4 left-4 bg-black/50 backdrop-blur-md text-white text-[10px] font-bold px-2 py-1 rounded uppercase tracking-wider flex items-center gap-1.5">
                    <span className="w-1.5 h-1.5 rounded-full bg-red-500 block animate-pulse"></span> LIVE
                  </div>
                </div>
              </div>

              {/* Modes */}
              <div className="grid grid-cols-4 gap-3">
                {[
                  { id: 'Manual', icon: <PlaySquare size={18} />, label: 'Manual' },
                  { id: 'LineFollow', icon: <Map size={18} />, label: 'Line Follow' },
                  { id: 'Obstacle', icon: <Shield size={18} />, label: 'Obstacle Avoid' },
                  { id: 'Auto', icon: <Activity size={18} />, label: 'Auto Patrol' },
                  { id: 'LEDControl', icon: <Lightbulb size={18} />, label: 'Đổi LED' },
                  { id: 'Mode6', icon: <HelpCircle size={18} />, label: 'Chưa có' },
                  { id: 'Mode7', icon: <HelpCircle size={18} />, label: 'Chưa có' },
                  { id: 'Mode8', icon: <HelpCircle size={18} />, label: 'Chưa có' }
                ].map(mode => (
                  <button
                    key={mode.id}
                    onClick={() => {
                      setActiveMode(mode.id);
                      if (ws && ws.readyState === WebSocket.OPEN) {
                        ws.send(JSON.stringify({ type: 'mode', mode: mode.id }));
                      }
                    }}
                    className={`p-3 rounded-xl border flex flex-col items-center gap-2 transition-all ${activeMode === mode.id
                      ? 'bg-blue-50 border-blue-200 text-blue-600 shadow-sm'
                      : 'bg-white border-slate-100 text-slate-500 hover:bg-slate-50'
                      }`}
                  >
                    {mode.icon}
                    <span className="text-[10px] font-semibold text-center leading-tight">{mode.label}</span>
                  </button>
                ))}
              </div>
            </div>

            {/* Right Column (Actions, Telemetry, Videos) */}
            <div className="col-span-12 lg:col-span-3 flex flex-col gap-6">

              {/* Quick Actions */}
              <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-100">
                <h2 className="text-sm font-semibold text-slate-500 uppercase tracking-wider mb-4">Quick Actions</h2>
                <div className="grid grid-cols-2 gap-3">
                  <button className="py-3 bg-blue-50 hover:bg-blue-100 text-blue-600 rounded-xl flex flex-col items-center gap-1.5 transition-colors">
                    <Lightbulb size={20} />
                    <span className="text-[10px] font-semibold">Headlight</span>
                  </button>
                  <button className="py-3 bg-slate-50 hover:bg-slate-100 text-slate-600 rounded-xl flex flex-col items-center gap-1.5 transition-colors border border-slate-100">
                    <Volume2 size={20} />
                    <span className="text-[10px] font-semibold">Buzzer</span>
                  </button>
                  <button className="py-3 bg-slate-50 hover:bg-slate-100 text-slate-600 rounded-xl flex flex-col items-center gap-1.5 transition-colors border border-slate-100">
                    <Sun size={20} />
                    <span className="text-[10px] font-semibold">LED</span>
                  </button>
                  <button className="py-3 bg-red-50 hover:bg-red-100 text-red-600 rounded-xl flex flex-col items-center gap-1.5 transition-colors">
                    <StopIcon size={20} />
                    <span className="text-[10px] font-semibold">Stop</span>
                  </button>
                </div>
              </div>

              {/* Telemetry Rings */}
              <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-100">
                <h2 className="text-sm font-semibold text-slate-500 uppercase tracking-wider mb-6">Telemetry</h2>

                <div className="grid grid-cols-2 gap-y-6 gap-x-4">
                  {/* Circular progress simulated with borders */}
                  <div className="flex flex-col items-center text-center">
                    <div className="w-14 h-14 rounded-full border-4 border-green-100 border-t-green-500 flex items-center justify-center mb-2">
                      <Battery size={16} className="text-slate-400" />
                    </div>
                    <span className="font-bold text-sm text-slate-700">{telemetry.battery_percent || 0}%</span>
                    <span className="text-[10px] text-slate-400">Battery</span>
                  </div>

                  <div className="flex flex-col items-center text-center">
                    <div className="w-14 h-14 rounded-full border-4 border-blue-100 border-t-blue-500 flex items-center justify-center mb-2">
                      <Activity size={16} className="text-slate-400" />
                    </div>
                    <span className="font-bold text-sm text-slate-700">{telemetry.speed || 0} cm/s</span>
                    <span className="text-[10px] text-slate-400">Speed</span>
                  </div>

                  <div className="flex flex-col items-center text-center">
                    <div className="w-14 h-14 rounded-full border-4 border-amber-100 border-t-amber-500 flex items-center justify-center mb-2">
                      <Zap size={16} className="text-slate-400" />
                    </div>
                    <span className="font-bold text-sm text-slate-700">{telemetry.voltage} V</span>
                    <span className="text-[10px] text-slate-400">Voltage</span>
                  </div>

                  <div className="flex flex-col items-center text-center">
                    <div className="w-14 h-14 rounded-full border-4 border-orange-100 border-t-orange-500 flex items-center justify-center mb-2">
                      <Thermometer size={16} className="text-slate-400" />
                    </div>
                    <span className="font-bold text-sm text-slate-700">{telemetry.temperature}°C</span>
                    <span className="text-[10px] text-slate-400">Temperature</span>
                  </div>
                </div>
              </div>

              {/* Recorded Videos */}
              <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-100 flex-1">
                <div className="flex justify-between items-end mb-4">
                  <h2 className="text-sm font-semibold text-slate-500 uppercase tracking-wider">Recorded Videos</h2>
                  <span className="text-[10px] text-blue-600 font-semibold cursor-pointer">View All</span>
                </div>

                <div className="space-y-3">
                  {[1, 2, 3].map((i) => (
                    <div key={i} className="flex gap-3 items-center p-2 hover:bg-slate-50 rounded-lg cursor-pointer transition-colors border border-transparent hover:border-slate-100">
                      <div className="w-16 h-10 bg-slate-200 rounded relative overflow-hidden flex items-center justify-center">
                        <PlaySquare size={14} className="text-white drop-shadow-md z-10" />
                        <div className="absolute inset-0 bg-black/20"></div>
                      </div>
                      <div className="flex-1">
                        <p className="text-[10px] font-semibold text-slate-700 truncate w-32">VID_20250526_14{i}0501.mp4</p>
                        <p className="text-[9px] text-slate-400 mt-0.5">May 26, 2025 • {45.2 + i} MB</p>
                      </div>
                    </div>
                  ))}
                </div>
              </div>

            </div>

          </div>
        </div>
      </div>
    </div>
  );
}
