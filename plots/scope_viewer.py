import tkinter as tk
from tkinter import filedialog, ttk
import csv
import re
import os
import sys

try:
    import matplotlib
    matplotlib.use("TkAgg")
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    import numpy as np
except ImportError:
    os.system(f"{sys.executable} -m pip install matplotlib numpy")
    import matplotlib
    matplotlib.use("TkAgg")
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    import numpy as np


class ScopeViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("Scope Viewer")
        self.root.geometry("1100x700")

        self.time_data = []
        self.voltage_data = []
        self.metadata = {}

        self.cursors = {"x1": None, "x2": None, "y1": None, "y2": None}
        self.cursor_lines = {"x1": None, "x2": None, "y1": None, "y2": None}
        self.cursor_mode = tk.StringVar(value="none")

        self._build_ui()

    def _build_ui(self):
        top = tk.Frame(self.root, pady=4, padx=6)
        top.pack(fill="x", side="top")

        tk.Button(top, text="Load CSV", command=self._load_file,
                  padx=10).pack(side="left", padx=4)
        self.file_label = tk.Label(top, text="no file loaded", fg="gray")
        self.file_label.pack(side="left", padx=6)

        main = tk.Frame(self.root)
        main.pack(fill="both", expand=True)

        left = tk.Frame(main, width=190, bd=1, relief="sunken")
        left.pack(side="left", fill="y", padx=(4, 0), pady=4)
        left.pack_propagate(False)

        tk.Label(left, text="Metadata", font=(None, 9, "bold")).pack(
            anchor="w", padx=8, pady=(8, 2))
        self.meta_frame = tk.Frame(left)
        self.meta_frame.pack(fill="x", padx=8)

        ttk.Separator(left, orient="horizontal").pack(fill="x", padx=8, pady=8)

        tk.Label(left, text="Cursors", font=(None, 9, "bold")).pack(
            anchor="w", padx=8, pady=(0, 4))
        for label, val in [("Off", "none"), ("X1", "x1"), ("X2", "x2"), ("Y1", "y1"), ("Y2", "y2")]:
            tk.Radiobutton(left, text=label, variable=self.cursor_mode, value=val,
                           command=self._on_cursor_mode_change).pack(anchor="w", padx=16, pady=1)

        tk.Button(left, text="Clear cursors", command=self._clear_cursors, padx=6).pack(
            padx=8, pady=(6, 0), fill="x")

        ttk.Separator(left, orient="horizontal").pack(fill="x", padx=8, pady=8)

        tk.Label(left, text="Measurements", font=(None, 9, "bold")).pack(
            anchor="w", padx=8, pady=(0, 4))
        self.meas_frame = tk.Frame(left)
        self.meas_frame.pack(fill="x", padx=8)
        self._build_measurements()

        plot_frame = tk.Frame(main)
        plot_frame.pack(side="left", fill="both", expand=True, padx=4, pady=4)

        self.fig, self.ax = plt.subplots()
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Voltage (V)")
        self.ax.grid(True, linestyle="--", alpha=0.4)
        self.ax.set_title("Load a CSV file to begin")

        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)
        self.canvas.mpl_connect("button_press_event", self._on_click)

        toolbar = NavigationToolbar2Tk(self.canvas, plot_frame)
        toolbar.update()

        self.fig.tight_layout(pad=1.5)

    def _build_measurements(self):
        for w in self.meas_frame.winfo_children():
            w.destroy()
        self.meas_labels = {}
        for label, key in [("dT", "delta_t"), ("dV", "delta_v"), ("Freq", "freq"),
                           ("X1", "x1"), ("X2", "x2"), ("Y1", "y1"), ("Y2", "y2")]:
            row = tk.Frame(self.meas_frame)
            row.pack(fill="x", pady=1)
            tk.Label(row, text=f"{label}:", width=5,
                     anchor="w", fg="gray").pack(side="left")
            lbl = tk.Label(row, text="—", anchor="w", fg="#0055cc")
            lbl.pack(side="left")
            self.meas_labels[key] = lbl

    def _load_file(self, filepath=None):
        if not filepath:
            filepath = filedialog.askopenfilename(
                title="Open CSV", filetypes=[("CSV files", "*.csv"), ("All files", "*.*")])
        if not filepath:
            return

        self.metadata = {}
        self.time_data = []
        self.voltage_data = []

        with open(filepath, "r") as f:
            lines = f.readlines()

        for line in lines:
            if line.startswith("#"):
                m = re.match(r"#(.+?):\s*(.+)", line.strip())
                if m:
                    self.metadata[m.group(1).strip()] = m.group(2).strip()

        reader = csv.reader(lines)
        for row in reader:
            if not row or row[0].startswith("#"):
                continue
            try:
                self.time_data.append(float(row[0]))
                self.voltage_data.append(float(row[1]))
            except (ValueError, IndexError):
                continue

        self.file_label.config(text=os.path.basename(filepath), fg="black")
        self._update_metadata_panel()
        self._plot()

    def _update_metadata_panel(self):
        for w in self.meta_frame.winfo_children():
            w.destroy()
        for key in ["Device Name", "Date Time", "Sample rate", "Samples", "Channel 1"]:
            if key in self.metadata:
                row = tk.Frame(self.meta_frame)
                row.pack(fill="x", pady=1)
                tk.Label(row, text=key.split()[-1] + ":", fg="gray", font=(None, 8),
                         width=7, anchor="w").pack(side="left")
                tk.Label(row, text=self.metadata[key], font=(None, 8),
                         anchor="w", wraplength=140, justify="left").pack(side="left")

    def _plot(self):
        self.ax.cla()
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Voltage (V)")
        self.ax.grid(True, linestyle="--", alpha=0.4)

        t = np.array(self.time_data)
        v = np.array(self.voltage_data)
        self.ax.plot(t, v, linewidth=0.9, color="#1a6fc4")
        self.ax.set_title(
            self.metadata.get("Device Name", "Oscilloscope") + "   " +
            self.metadata.get("Date Time", ""))

        self.cursor_lines = {"x1": None, "x2": None, "y1": None, "y2": None}
        self._redraw_cursors()
        self.fig.tight_layout(pad=1.5)
        self.canvas.draw()

    def _on_cursor_mode_change(self):
        self.root.config(
            cursor="crosshair" if self.cursor_mode.get() != "none" else "")

    def _on_click(self, event):
        if event.inaxes != self.ax:
            return
        mode = self.cursor_mode.get()
        if mode == "none":
            return
        if mode.startswith("x"):
            self.cursors[mode] = event.xdata
        else:
            self.cursors[mode] = event.ydata
        self._redraw_cursors()
        self._update_measurements()
        self.canvas.draw()

    def _redraw_cursors(self):
        colors = {"x1": "red", "x2": "orangered",
                  "y1": "green", "y2": "purple"}
        for key, val in self.cursors.items():
            if self.cursor_lines[key] is not None:
                try:
                    self.cursor_lines[key].remove()
                except Exception:
                    pass
                self.cursor_lines[key] = None
            if val is not None:
                if key.startswith("x"):
                    line = self.ax.axvline(
                        val, color=colors[key], linewidth=1.2, linestyle="--", alpha=0.85)
                    self.ax.text(val, self.ax.get_ylim()[1], f" {key.upper()}",
                                 color=colors[key], fontsize=8, va="top")
                else:
                    line = self.ax.axhline(
                        val, color=colors[key], linewidth=1.2, linestyle="--", alpha=0.85)
                    self.ax.text(self.ax.get_xlim()[0], val, f"{key.upper()} ",
                                 color=colors[key], fontsize=8, ha="left", va="bottom")
                self.cursor_lines[key] = line

    def _clear_cursors(self):
        self.cursors = {"x1": None, "x2": None, "y1": None, "y2": None}
        self.cursor_mode.set("none")
        self.root.config(cursor="")
        self._plot()
        self._update_measurements()

    def _update_measurements(self):
        def fmt_time(v):
            if v is None:
                return "—"
            a = abs(v)
            if a < 1e-6:
                return f"{v*1e9:.3f} ns"
            if a < 1e-3:
                return f"{v*1e6:.3f} us"
            if a < 1:
                return f"{v*1e3:.3f} ms"
            return f"{v:.4f} s"

        def fmt_volt(v):
            return "—" if v is None else f"{v:.4f} V"

        x1, x2 = self.cursors["x1"], self.cursors["x2"]
        y1, y2 = self.cursors["y1"], self.cursors["y2"]

        self.meas_labels["x1"].config(text=fmt_time(x1))
        self.meas_labels["x2"].config(text=fmt_time(x2))
        self.meas_labels["y1"].config(text=fmt_volt(y1))
        self.meas_labels["y2"].config(text=fmt_volt(y2))

        if x1 is not None and x2 is not None:
            dt = abs(x2 - x1)
            self.meas_labels["delta_t"].config(text=fmt_time(dt))
            if dt > 0:
                f = 1.0 / dt
                if f >= 1e6:
                    self.meas_labels["freq"].config(text=f"{f/1e6:.3f} MHz")
                elif f >= 1e3:
                    self.meas_labels["freq"].config(text=f"{f/1e3:.3f} kHz")
                else:
                    self.meas_labels["freq"].config(text=f"{f:.3f} Hz")
            else:
                self.meas_labels["freq"].config(text="—")
        else:
            self.meas_labels["delta_t"].config(text="—")
            self.meas_labels["freq"].config(text="—")

        if y1 is not None and y2 is not None:
            self.meas_labels["delta_v"].config(text=fmt_volt(abs(y2 - y1)))
        else:
            self.meas_labels["delta_v"].config(text="—")


if __name__ == "__main__":
    root = tk.Tk()
    app = ScopeViewer(root)
    if len(sys.argv) > 1:
        app._load_file(sys.argv[1])
    root.mainloop()
