import os, textwrap, tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageDraw, ImageFont, ImageEnhance, ImageTk
import matplotlib.font_manager as fm

class GDTriggerApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("GD Trigger Generator")
        self.geometry("800x520")
        self.resizable(False, False)

        self.image_path = tk.StringVar()
        self.trigger_text = tk.StringVar(value="Execute")
        self.font_path = tk.StringVar(value=self.find_pusab_font())
        
        self.hue_val, self.sat_val, self.bright_val = tk.DoubleVar(value=0.0), tk.DoubleVar(value=1.0), tk.DoubleVar(value=1.0)
        self.preview_image_pil, self.preview_photo_tk, self._debounce_job = None, None, None

        self.setup_ui()

    def find_pusab_font(self):
        for path in fm.findSystemFonts(fontpaths=None, fontext='otf'):
            if "pusab" in os.path.basename(path).lower(): return path
        for local_file in ["pusab.otf", "Pusab.otf", "pusab.ttf", "Pusab.ttf"]:
            if os.path.exists(local_file): return os.path.abspath(local_file)
        return ""

    def setup_ui(self):
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)

        controls_frame = ttk.LabelFrame(self, text=" Configuration ", padding=15)
        controls_frame.grid(row=0, column=0, sticky="nsew", padx=15, pady=15)

        ttk.Label(controls_frame, text="Base Image:").grid(row=0, column=0, sticky="w", pady=(0, 2))
        img_box = ttk.Frame(controls_frame)
        img_box.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(0, 10))
        ttk.Entry(img_box, textvariable=self.image_path).pack(side="left", fill="x", expand=True, padx=(0, 5))
        ttk.Button(img_box, text="Browse...", command=self.browse_image).pack(side="right")

        ttk.Label(controls_frame, text="Trigger Text:").grid(row=2, column=0, sticky="w", pady=(0, 2))
        text_entry = ttk.Entry(controls_frame, textvariable=self.trigger_text)
        text_entry.grid(row=3, column=0, columnspan=2, sticky="ew", pady=(0, 10))
        text_entry.bind("<KeyRelease>", self.on_text_keyrelease)

        ttk.Label(controls_frame, text="Pusab Font:").grid(row=4, column=0, sticky="w", pady=(0, 2))
        font_box = ttk.Frame(controls_frame)
        font_box.grid(row=5, column=0, columnspan=2, sticky="ew", pady=(0, 10))
        ttk.Entry(font_box, textvariable=self.font_path).pack(side="left", fill="x", expand=True, padx=(0, 5))
        ttk.Button(font_box, text="Browse...", command=self.browse_font).pack(side="right")

        ttk.Separator(controls_frame, orient="horizontal").grid(row=6, column=0, columnspan=2, sticky="ew", pady=10)

        ttk.Label(controls_frame, text="Hue Shift:").grid(row=7, column=0, sticky="w")
        hue_slider = ttk.Scale(controls_frame, from_=-180, to=180, variable=self.hue_val)
        hue_slider.grid(row=7, column=1, sticky="ew", padx=(5, 0))
        hue_slider.bind("<ButtonRelease-1>", lambda e: self.update_preview())

        ttk.Label(controls_frame, text="Saturation:").grid(row=8, column=0, sticky="w", pady=5)
        sat_slider = ttk.Scale(controls_frame, from_=0.0, to=2.0, variable=self.sat_val)
        sat_slider.grid(row=8, column=1, sticky="ew", padx=(5, 0))
        sat_slider.bind("<ButtonRelease-1>", lambda e: self.update_preview())

        ttk.Label(controls_frame, text="Brightness:").grid(row=9, column=0, sticky="w")
        bright_slider = ttk.Scale(controls_frame, from_=0.2, to=2.0, variable=self.bright_val)
        bright_slider.grid(row=9, column=1, sticky="ew", padx=(5, 0))
        bright_slider.bind("<ButtonRelease-1>", lambda e: self.update_preview())

        ttk.Button(controls_frame, text="Export Trigger PNG", command=self.save_trigger).grid(row=10, column=0, columnspan=2, sticky="ew", pady=(20, 0))

        preview_frame = ttk.LabelFrame(self, text=" Live Preview ", padding=15)
        preview_frame.grid(row=0, column=1, sticky="nsew", padx=(0, 15), pady=15)

        self.canvas = tk.Canvas(preview_frame, bg="#1a1a1a", highlightthickness=1, highlightbackground="#333333")
        self.canvas.pack(fill="both", expand=True)

    def on_text_keyrelease(self, event):
        if self._debounce_job: self.after_cancel(self._debounce_job)
        self._debounce_job = self.after(300, self.update_preview)

    def browse_image(self):
        path = filedialog.askopenfilename(title="Select Base Image", filetypes=[("Image Files", "*.png *.jpg *.jpeg *.webp *.bmp")])
        if path: self.image_path.set(path); self.update_preview()

    def browse_font(self):
        path = filedialog.askopenfilename(title="Select Pusab Font File", filetypes=[("Font Files", "*.otf *.ttf")])
        if path: self.font_path.set(path); self.update_preview()

    def apply_color_adjustments(self, img):
        img = img.convert("RGBA")
        img = ImageEnhance.Color(img).enhance(self.sat_val.get())
        img = ImageEnhance.Brightness(img).enhance(self.bright_val.get())

        hue_shift = self.hue_val.get()
        if abs(hue_shift) > 0.01:
            r, g, b, a = img.split()
            h, s, v = Image.merge("RGB", (r, g, b)).convert("HSV").split()
            h = h.point(lambda p: (p + int((hue_shift / 360.0) * 255)) % 256)
            r_s, g_s, b_s = Image.merge("HSV", (h, s, v)).convert("RGB").split()
            img = Image.merge("RGBA", (r_s, g_s, b_s, a))

        return img

    def render_trigger_image(self):
        if not self.image_path.get() or not os.path.exists(self.image_path.get()): return None
        TOTAL_WIDTH, PADDING_Y = 90, 6

        try: base_img = Image.open(self.image_path.get())
        except Exception: return None

        base_img = self.apply_color_adjustments(base_img)

        text = self.trigger_text.get().strip()
        wrapped_text = textwrap.fill(text, width=12, break_long_words=False, break_on_hyphens=False) if text else ""

        fpath, font_size, font = self.font_path.get(), 18, None
        draw_dummy = ImageDraw.Draw(Image.new("RGBA", (1, 1)))

        if wrapped_text:
            while font_size > 6:
                font = ImageFont.truetype(fpath, font_size) if fpath and os.path.exists(fpath) else ImageFont.load_default()
                if not fpath or not os.path.exists(fpath): break
                bbox = draw_dummy.multiline_textbbox((0, 0), wrapped_text, font=font, align="center")
                if bbox[2] - bbox[0] <= TOTAL_WIDTH: break
                font_size -= 1

            bbox = draw_dummy.multiline_textbbox((0, 0), wrapped_text, font=font, align="center")
            text_top_offset, text_h = bbox[1], bbox[3] - bbox[1]
        else: text_h, text_top_offset = 0, 0

        target_icon_w = 65
        icon_h = int(target_icon_w * (base_img.height / base_img.width))
        scaled_icon = base_img.resize((target_icon_w, icon_h), Image.NEAREST)

        spacing = 6 if wrapped_text else 0
        canvas_h = PADDING_Y + text_h + spacing + icon_h + PADDING_Y
        canvas = Image.new("RGBA", (TOTAL_WIDTH, canvas_h), (0, 0, 0, 0))
        draw = ImageDraw.Draw(canvas)

        if wrapped_text:
            draw.multiline_text((TOTAL_WIDTH // 2, PADDING_Y - text_top_offset), wrapped_text, font=font, fill=(255, 255, 255, 255), align="center", anchor="ma")

        canvas.paste(scaled_icon, ((TOTAL_WIDTH - target_icon_w) // 2, PADDING_Y + text_h + spacing), scaled_icon)
        return canvas

    def update_preview(self):
        self._debounce_job = None
        self.preview_image_pil = self.render_trigger_image()
        self.canvas.delete("all")

        if self.preview_image_pil is None:
            self.canvas.create_text(self.canvas.winfo_width() // 2 or 150, self.canvas.winfo_height() // 2 or 180, text="Please select an image file...", fill="#777777", font=("Arial", 10))
            return

        display_img = self.preview_image_pil.resize((self.preview_image_pil.width * 3, self.preview_image_pil.height * 3), Image.NEAREST)
        self.preview_photo_tk = ImageTk.PhotoImage(display_img)
        self.canvas.create_image(self.canvas.winfo_width() // 2, self.canvas.winfo_height() // 2, image=self.preview_photo_tk)

    def save_trigger(self):
        if self.preview_image_pil is None:
            messagebox.showwarning("Warning", "No trigger image generated to save!")
            return

        text = self.trigger_text.get().strip().replace(" ", "_") or "gd_trigger"
        save_path = filedialog.asksaveasfilename(title="Save Trigger PNG", initialfile=f"{text}.png", defaultextension=".png", filetypes=[("PNG Files", "*.png")])
        if save_path:
            self.preview_image_pil.save(save_path)
            messagebox.showinfo("Success", f"Trigger successfully saved to:\n{save_path}")

if __name__ == "__main__":
    GDTriggerApp().mainloop()