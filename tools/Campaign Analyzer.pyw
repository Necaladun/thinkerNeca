# thinkerNeca Campaign Analyzer — Windows double-click GUI
#
# Install BOTH this file and campaign_report.py beside one another in:
#   C:\smac\smac-game\campaigns\
#
# Double-click this .pyw file. Requires standard Python with Tkinter (no pip).
# The command-line reporter also remains available independently.

from pathlib import Path
import os
import queue
import threading
import webbrowser

import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from campaign_report import generate, scan_campaigns


class CampaignPicker(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("thinkerNeca — Campaign Analyzer")
        self.geometry("1110x680")
        self.minsize(790, 520)
        # Resolve relative to this script, never to an arbitrary PowerShell cwd.
        self.campaigns_root = Path(__file__).resolve().parent
        self.all_rows = []
        self.rows_by_id = {}
        self.result_queue = queue.Queue()
        self.busy = False
        self.search = tk.StringVar()
        self.search.trace_add("write", lambda *_: self.show_rows())
        self.path_label = tk.StringVar()
        self.detail_label = tk.StringVar()
        self.info_label = tk.StringVar(value="Scanning campaigns…")
        self._build()
        self.refresh()
        self.after(125, self._poll_results)

    def _build(self):
        outer = ttk.Frame(self, padding=14)
        outer.pack(fill="both", expand=True)
        ttk.Label(outer, text="Campaign Analyzer", font=("Segoe UI", 18, "bold")).pack(anchor="w")
        ttk.Label(
            outer,
            text="Choose a recorded game/session, then generate its offline report.",
        ).pack(anchor="w", pady=(0, 12))

        path_row = ttk.Frame(outer)
        path_row.pack(fill="x", pady=(0, 9))
        ttk.Label(path_row, text="Campaigns folder:").pack(side="left")
        ttk.Label(path_row, textvariable=self.path_label, padding=(8, 0)).pack(
            side="left", fill="x", expand=True
        )
        ttk.Button(path_row, text="Choose folder…", command=self.choose_folder).pack(
            side="right", padx=(7, 0)
        )
        ttk.Button(path_row, text="Refresh", command=self.refresh).pack(side="right")

        search_row = ttk.Frame(outer)
        search_row.pack(fill="x", pady=(0, 7))
        ttk.Label(search_row, text="Filter:").pack(side="left")
        ttk.Entry(search_row, textvariable=self.search).pack(
            side="left", fill="x", expand=True, padx=(9, 0)
        )

        table_frame = ttk.Frame(outer)
        table_frame.pack(fill="both", expand=True)
        columns = ("campaign_name", "player", "years", "turns", "bases", "units", "status", "updated")
        self.tree = ttk.Treeview(
            table_frame, columns=columns, show="headings", selectmode="browse", height=12
        )
        specs = (
            ("campaign_name", "Campaign", 220, "w"),
            ("player", "Played faction / mode", 175, "w"),
            ("years", "Years recorded", 125, "w"),
            ("turns", "Turns", 75, "center"),
            ("bases", "Bases", 60, "center"),
            ("units", "Units", 60, "center"),
            ("status", "Status", 95, "center"),
            ("updated", "Last updated (local)", 160, "w"),
        )
        for key, heading, width, anchor in specs:
            self.tree.heading(key, text=heading)
            self.tree.column(key, width=width, minwidth=60, anchor=anchor, stretch=(key == "player"))
        vertical = ttk.Scrollbar(table_frame, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=vertical.set)
        self.tree.pack(side="left", fill="both", expand=True)
        vertical.pack(side="right", fill="y")
        self.tree.bind("<<TreeviewSelect>>", lambda _event: self.show_details())
        self.tree.bind("<Double-1>", lambda _event: self.generate_selected())

        details = ttk.LabelFrame(outer, text="Selected recording", padding=(12, 9))
        details.pack(fill="x", pady=(12, 8))
        ttk.Label(
            details, textvariable=self.detail_label, justify="left", anchor="w",
            wraplength=1020,
        ).pack(fill="x")

        buttons = ttk.Frame(outer)
        buttons.pack(fill="x", pady=(6, 5))
        self.generate_button = ttk.Button(
            buttons, text="Generate and open report", command=self.generate_selected
        )
        self.generate_button.pack(side="left")
        self.existing_button = ttk.Button(
            buttons, text="Open existing report", command=self.open_existing
        )
        self.existing_button.pack(side="left", padx=8)
        self.folder_button = ttk.Button(
            buttons, text="Open campaign folder", command=self.open_folder
        )
        self.folder_button.pack(side="left")
        ttk.Label(
            outer, textvariable=self.info_label, anchor="w",
        ).pack(fill="x", pady=(9, 0))

    def choose_folder(self):
        chosen = filedialog.askdirectory(
            parent=self, title="Select campaigns folder",
            initialdir=str(self.campaigns_root),
            mustexist=True,
        )
        if chosen:
            self.campaigns_root = Path(chosen)
            self.refresh()

    def refresh(self):
        previously = self.selected_path()
        try:
            self.all_rows = scan_campaigns(self.campaigns_root)
        except OSError as exc:
            messagebox.showerror("Cannot scan campaigns", str(exc), parent=self)
            self.all_rows = []
        self.path_label.set(str(self.campaigns_root))
        self.show_rows(preferred=previously)
        self.info_label.set(
            f"{len(self.all_rows)} session(s) found. "
            "Select a recording; use Refresh to see new turns or games."
        )

    def selected_path(self):
        selected = self.tree.selection()
        if not selected:
            return None
        info = self.rows_by_id.get(selected[0])
        if not info:
            return None
        return info["campaign_dir"] / "sessions" / info["session_id"]

    def show_rows(self, preferred=None):
        if preferred is None:
            preferred = self.selected_path()
        self.tree.delete(*self.tree.get_children())
        self.rows_by_id.clear()
        term = self.search.get().strip().casefold()
        for index, row in enumerate(self.all_rows):
            haystack = " ".join(
                str(row[key]) for key in (
                    "folder", "campaign_name", "player", "years", "status", "started",
                    "updated", "source_save", "factions", "session_id",
                )
            ).casefold()
            if term and term not in haystack:
                continue
            iid = str(index)
            self.rows_by_id[iid] = row
            self.tree.insert(
                "", "end", iid=iid,
                values=tuple(row[key] for key in (
                    "campaign_name", "player", "years", "turns", "bases",
                    "units", "status", "updated",
                )),
            )
            if preferred == row["campaign_dir"] / "sessions" / row["session_id"]:
                self.tree.selection_set(iid)
        if not self.tree.selection() and self.tree.get_children():
            self.tree.selection_set(self.tree.get_children()[0])
        self.show_details()

    def selected_row(self):
        selected = self.tree.selection()
        return self.rows_by_id.get(selected[0]) if selected else None

    def show_details(self):
        row = self.selected_row()
        if not row:
            self.detail_label.set(
                "No campaign selected. Record a game with telemetry_enabled=1, "
                "or choose another campaigns folder."
            )
        elif row["error"]:
            self.detail_label.set(
                f"Folder: {row['folder']}  ·  Session: {row['session_id']}\n"
                f"Cannot read recording: {row['error']}"
            )
        else:
            self.detail_label.set(
                f"Campaign: {row['campaign_name']}  ·  Session: {row['session_id']}\n"
                f"Folder: {row['folder']}\n"
                f"Started: {row['started']}  ·  Status: {row['status']}  ·  "
                f"Source save: {row['source_save']}\n"
                f"Alive factions: {row['factions'] or 'Not recorded'}"
            )
        self._update_buttons()

    def _update_buttons(self):
        row = self.selected_row()
        valid = row is not None and not row["error"] and not self.busy
        self.generate_button.configure(state="normal" if valid else "disabled")
        self.existing_button.configure(state="normal" if row and not self.busy else "disabled")
        self.folder_button.configure(state="normal" if row and not self.busy else "disabled")

    def generate_selected(self):
        row = self.selected_row()
        if self.busy or not row:
            return
        if row["error"]:
            messagebox.showerror("Invalid recording", row["error"], parent=self)
            return
        self.busy = True
        self._update_buttons()
        self.info_label.set("Generating report…")
        thread = threading.Thread(
            target=self._generate_worker,
            args=(row["campaign_dir"], row["session_id"]),
            daemon=True,
        )
        thread.start()

    def _generate_worker(self, campaign_dir, session_id):
        try:
            output = generate(campaign_dir, session_id=session_id)
            self.result_queue.put(("success", output / "index.html"))
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.result_queue.put(("error", str(exc)))
        except Exception as exc:
            # No terminal is attached to .pyw; show unexpected failures too.
            self.result_queue.put(("error", f"{type(exc).__name__}: {exc}"))

    def _poll_results(self):
        while True:
            try:
                kind, value = self.result_queue.get_nowait()
            except queue.Empty:
                break
            self.busy = False
            self._update_buttons()
            if kind == "success":
                self.info_label.set(f"Report created: {value}")
                self._open_path(value)
            else:
                self.info_label.set("Report generation failed.")
                messagebox.showerror("Report generation failed", value, parent=self)
        self.after(125, self._poll_results)

    def existing_report_path(self):
        row = self.selected_row()
        if not row:
            return None
        return row["campaign_dir"] / "reports" / row["session_id"] / "index.html"

    def open_existing(self):
        path = self.existing_report_path()
        if path and path.is_file():
            self._open_path(path)
        else:
            messagebox.showinfo(
                "No existing report",
                "This session has no generated report yet. "
                "Choose 'Generate and open report'.",
                parent=self,
            )

    def open_folder(self):
        row = self.selected_row()
        if row:
            self._open_path(row["campaign_dir"])

    def _open_path(self, path):
        try:
            if os.name == "nt":
                os.startfile(str(path))
            else:
                webbrowser.open(path.as_uri())
        except (OSError, ValueError) as exc:
            messagebox.showerror("Cannot open", str(exc), parent=self)


if __name__ == "__main__":
    CampaignPicker().mainloop()
