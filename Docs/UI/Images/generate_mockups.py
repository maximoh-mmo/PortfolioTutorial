import os
from PIL import Image, ImageDraw, ImageFont

OUTPUT_DIR = r"e:\Unreal Projects\PortfolioTutorial\Docs\UI\Images"
os.makedirs(OUTPUT_DIR, exist_ok=True)

FONT_PATH = r"C:\Windows\Fonts\segoeui.ttf"
FONT_BOLD_PATH = r"C:\Windows\Fonts\segoeuib.ttf"
if not os.path.exists(FONT_BOLD_PATH):
    FONT_BOLD_PATH = FONT_PATH

def get_font(size, bold=False):
    path = FONT_BOLD_PATH if bold else FONT_PATH
    try:
        return ImageFont.truetype(path, size)
    except:
        return ImageFont.load_default()

# Color Palette
BG_DARK = (13, 17, 23)
PANEL_BG = (22, 27, 34)
PANEL_BORDER = (48, 54, 61)
TEXT_WHITE = (240, 246, 252)
TEXT_MUTED = (139, 148, 158)
ACCENT_GOLD = (229, 169, 60)
ACCENT_CYAN = (56, 189, 248)
ACCENT_GREEN = (34, 197, 94)
ACCENT_RED = (239, 68, 68)
ACCENT_PURPLE = (168, 85, 247)
ACCENT_AMBER = (245, 158, 11)
BUTTON_BG = (33, 38, 45)
BUTTON_BORDER = (80, 88, 98)

def draw_rounded_panel(draw, xy, fill=PANEL_BG, outline=PANEL_BORDER, radius=8, width=1):
    draw.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline, width=width)

def draw_header(draw, title, subtitle, width=1920):
    f_title = get_font(28, bold=True)
    f_sub = get_font(16)
    draw.text((40, 30), title, fill=TEXT_WHITE, font=f_title)
    draw.text((40, 68), subtitle, fill=ACCENT_CYAN, font=f_sub)
    draw.line([(40, 95), (width - 40, 95)], fill=PANEL_BORDER, width=2)

def draw_button(draw, xy, text, is_primary=False, f_size=15):
    fill = (180, 83, 9) if is_primary else BUTTON_BG
    border = ACCENT_GOLD if is_primary else BUTTON_BORDER
    text_color = (255, 255, 255) if is_primary else TEXT_WHITE
    draw_rounded_panel(draw, xy, fill=fill, outline=border, radius=6, width=1)
    f = get_font(f_size, bold=True)
    bbox = draw.textbbox((0, 0), text, font=f)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    cx = (xy[0] + xy[2] - w) // 2
    cy = (xy[1] + xy[3] - h) // 2
    draw.text((cx, cy), text, fill=text_color, font=f)

# ==========================================================
# 1. HUD LAYOUT (1920 x 1080)
# ==========================================================
def generate_hud_layout():
    img = Image.new("RGB", (1920, 1080), BG_DARK)
    draw = ImageDraw.Draw(img)

    # Grid guide lines
    for x in range(0, 1920, 160):
        draw.line([(x, 0), (x, 1080)], fill=(20, 25, 33), width=1)
    for y in range(0, 1080, 120):
        draw.line([(0, y), (1920, y)], fill=(20, 25, 33), width=1)

    draw_header(draw, "PROPOSED LAYOUT: COMBAT HUD (WBP_HUD)", "Layer: Viewport / HUD | Safe Margins: 40px | Interactive Elements: Game Input Mode")

    # Safe Area Outline
    draw.rectangle([40, 110, 1880, 1040], outline=(40, 48, 60), width=1)
    draw.text((45, 115), "SCREEN SAFE BOUNDARY [1920x1080]", fill=(70, 80, 95), font=get_font(12))

    # Top-Left: Player Status Frame & Buff Bar
    p_tl = (60, 140, 480, 320)
    draw_rounded_panel(draw, p_tl, fill=(18, 22, 28))
    draw.text((80, 155), "PLAYER FRAME (WBP_PlayerHealthBar)", fill=ACCENT_GOLD, font=get_font(15, bold=True))
    draw.text((80, 180), "Level 42  |  Shadowblade Assassin", fill=TEXT_MUTED, font=get_font(13))
    
    # Health bar
    draw.rectangle([80, 205, 460, 225], fill=(45, 20, 20), outline=(100, 30, 30))
    draw.rectangle([80, 205, 390, 225], fill=ACCENT_RED)
    draw.text((230, 208), "3,450 / 4,200 HP", fill=TEXT_WHITE, font=get_font(12, bold=True))

    # XP bar
    draw.rectangle([80, 235, 460, 245], fill=(20, 35, 45), outline=(30, 60, 80))
    draw.rectangle([80, 235, 330, 245], fill=ACCENT_CYAN)
    draw.text((245, 235), "XP: 68%", fill=TEXT_WHITE, font=get_font(9))

    # Buff Bar (WBP_BuffBar)
    draw.text((80, 260), "STATUS EFFECT & BUFF BAR (WBP_BuffBar)", fill=TEXT_MUTED, font=get_font(12, bold=True))
    buffs = [
        ("HASTE", ACCENT_CYAN, "12s"),
        ("MIGHT", ACCENT_GOLD, "45s"),
        ("BURN", ACCENT_RED, "4s"),
        ("SLOW", ACCENT_AMBER, "2s")
    ]
    for i, (bname, bcol, btime) in enumerate(buffs):
        bx = 80 + i * 55
        draw_rounded_panel(draw, (bx, 280, bx + 45, 310), fill=(25, 30, 40), outline=bcol, radius=4)
        draw.text((bx + 6, 284), bname[:3], fill=bcol, font=get_font(10, bold=True))
        draw.text((bx + 10, 296), btime, fill=TEXT_WHITE, font=get_font(9))

    # Top-Center: Target HUD (WBP_TargetHUD)
    p_tc = (760, 130, 1160, 230)
    draw_rounded_panel(draw, p_tc, fill=(24, 20, 24), outline=(100, 50, 70))
    draw.text((780, 142), "TARGET FRAME (WBP_TargetHUD)", fill=(244, 114, 182), font=get_font(14, bold=True))
    draw.text((780, 162), "Gorgon Behemoth [Elite Boss - Lv 45]", fill=TEXT_WHITE, font=get_font(13, bold=True))
    draw.rectangle([780, 188, 1140, 208], fill=(50, 20, 25), outline=(120, 30, 40))
    draw.rectangle([780, 188, 1050, 208], fill=ACCENT_RED)
    draw.text((910, 191), "78,400 / 92,000 HP (85%)", fill=TEXT_WHITE, font=get_font(12, bold=True))

    # Top-Right: Quest Tracker (WBP_QuestTracker)
    p_tr = (1520, 140, 1860, 460)
    draw_rounded_panel(draw, p_tr, fill=(18, 22, 28))
    draw.text((1540, 155), "QUEST TRACKER (WBP_QuestTracker)", fill=ACCENT_GOLD, font=get_font(14, bold=True))
    draw.line([(1540, 180), (1840, 180)], fill=PANEL_BORDER)
    
    # Quest 1
    draw.text((1540, 195), "In Search of the Ancients", fill=ACCENT_GOLD, font=get_font(13, bold=True))
    draw.text((1540, 215), "Investigate the sunken ruin chamber.", fill=TEXT_MUTED, font=get_font(12))
    draw.text((1550, 235), "- Defeat Corrupted Sentinels: 4/5", fill=TEXT_WHITE, font=get_font(12))
    draw.text((1550, 255), "- Collect Echo Shards: 2/3", fill=TEXT_WHITE, font=get_font(12))

    # Quest 2
    draw.text((1540, 290), "Outpost Defense Drill", fill=ACCENT_GOLD, font=get_font(13, bold=True))
    draw.text((1540, 310), "Repel invading border prowlers.", fill=TEXT_MUTED, font=get_font(12))
    draw.text((1550, 330), "[COMPLETED] Speak with Captain Vane", fill=ACCENT_GREEN, font=get_font(12))

    # Center: Decal Reticle & Combat Focus
    draw.ellipse([935, 515, 985, 565], outline=(80, 100, 120), width=2)
    draw.line([(960, 500), (960, 510)], fill=(80, 100, 120), width=2)
    draw.line([(960, 570), (960, 580)], fill=(80, 100, 120), width=2)
    draw.line([(920, 540), (930, 540)], fill=(80, 100, 120), width=2)
    draw.line([(990, 540), (1000, 540)], fill=(80, 100, 120), width=2)
    draw.text((915, 585), "Combat Reticle", fill=(80, 100, 120), font=get_font(11))

    # Bottom-Center: Cast Bar & Ability Action Bar
    # Cast bar (WBP_CastBar)
    p_cast = (780, 840, 1140, 885)
    draw_rounded_panel(draw, p_cast, fill=(20, 25, 35), outline=ACCENT_CYAN)
    draw.rectangle([782, 842, 1050, 883], fill=(30, 80, 120))
    draw.text((800, 852), "Casting: Void Nova", fill=TEXT_WHITE, font=get_font(13, bold=True))
    draw.text((1070, 852), "1.8s", fill=ACCENT_CYAN, font=get_font(13, bold=True))

    # Ability Bar (WBP_AbilityBar)
    p_ab = (680, 905, 1240, 1025)
    draw_rounded_panel(draw, p_ab, fill=(18, 22, 28))
    draw.text((700, 915), "ABILITY ACTION BAR (WBP_AbilityBar)", fill=ACCENT_GOLD, font=get_font(12, bold=True))
    
    slots = ["1 (Basic)", "2 (AoE)", "3 (Cone)", "4 (Dash)", "R (Ultimate)"]
    for i, s in enumerate(slots):
        sx = 700 + i * 105
        draw_rounded_panel(draw, (sx, 935, sx + 95, 1010), fill=BUTTON_BG, outline=BUTTON_BORDER, radius=6)
        draw.text((sx + 8, 942), s, fill=TEXT_WHITE, font=get_font(11, bold=True))
        if i == 1:
            draw.text((sx + 35, 965), "4.2s", fill=ACCENT_AMBER, font=get_font(15, bold=True))
        elif i == 4:
            draw.text((sx + 30, 965), "READY", fill=ACCENT_GREEN, font=get_font(12, bold=True))

    # Bottom-Right: Paperdoll Durability Warning & System Keys
    p_br = (1560, 880, 1860, 1025)
    draw_rounded_panel(draw, p_br, fill=(25, 20, 20), outline=ACCENT_AMBER)
    draw.text((1580, 895), "DURABILITY ALERT (WBP_DurabilityWarning)", fill=ACCENT_AMBER, font=get_font(12, bold=True))
    draw.text((1580, 920), "! Warning: Weapon at 18% Durability", fill=ACCENT_AMBER, font=get_font(12))
    draw.text((1580, 940), "! Boots Malfunctioning (0% - 50% DEF)", fill=ACCENT_RED, font=get_font(12, bold=True))
    draw.line([(1580, 965), (1840, 965)], fill=PANEL_BORDER)
    draw.text((1580, 975), "[I] Inventory   [C] Character   [ESC] Menu", fill=TEXT_MUTED, font=get_font(12))

    img.save(os.path.join(OUTPUT_DIR, "layout_hud.png"))
    print("Generated layout_hud.png")

# ==========================================================
# 2. DEATH SCREEN LAYOUT (1920 x 1080)
# ==========================================================
def generate_death_screen_layout():
    img = Image.new("RGB", (1920, 1080), (8, 5, 8))
    draw = ImageDraw.Draw(img)

    # Vignette red gradient simulation
    for i in range(100):
        alpha = int(120 * (1 - i / 100))
        draw.rectangle([i*8, i*4, 1920 - i*8, 1080 - i*4], outline=(40, 10, 15), width=2)

    draw_header(draw, "PROPOSED LAYOUT: DEATH & RESPAWN MODAL (WBP_DeathScreen)", "Layer: Modal | Mode: Menu (Input Captured / World Continues Ticking)")

    # Center Modal Card
    card = (610, 200, 1310, 880)
    draw_rounded_panel(draw, card, fill=(22, 16, 20), outline=ACCENT_RED, radius=12, width=2)

    # Banner Header
    draw.rectangle([612, 202, 1308, 290], fill=(50, 15, 20))
    draw.text((790, 225), "YOU HAVE FALLEN", fill=ACCENT_RED, font=get_font(36, bold=True))
    draw.text((820, 265), "DEFEATED IN COMBAT", fill=(200, 150, 160), font=get_font(15))

    # Killer identity
    draw.text((660, 320), "Killer Identification:", fill=TEXT_MUTED, font=get_font(14))
    draw_rounded_panel(draw, (660, 345, 1260, 415), fill=(28, 20, 24), outline=(80, 30, 40))
    draw.text((680, 360), "Slain by: Gorgon Berserker (Elite Tier 3)", fill=TEXT_WHITE, font=get_font(18, bold=True))
    draw.text((680, 388), "Fatal blow: Heavy Slam (Physical Crushing)", fill=ACCENT_AMBER, font=get_font(13))

    # Penalty & Durability wear
    draw.text((660, 440), "Death Penalties & Equipment Wear:", fill=TEXT_MUTED, font=get_font(14))
    draw_rounded_panel(draw, (660, 465, 1260, 560), fill=(28, 20, 24), outline=(80, 30, 40))
    draw.text((680, 480), "Equipment Durability Loss: -15% on All Equipped Items", fill=ACCENT_AMBER, font=get_font(16, bold=True))
    draw.text((680, 508), "Items at 0% will enter Malfunctioning state (50% penalty).", fill=TEXT_MUTED, font=get_font(13))
    draw.text((680, 530), "No item drops incurred (PvE Sanctuary rules).", fill=ACCENT_GREEN, font=get_font(13))

    # Respawn destination
    draw.text((660, 580), "Designated Sanctuary Point:", fill=TEXT_MUTED, font=get_font(14))
    draw_rounded_panel(draw, (660, 605, 1260, 665), fill=(28, 20, 24), outline=(80, 30, 40))
    draw.text((680, 622), "Sanctuary: Outpost Alpha (Forward Recon Camp)", fill=ACCENT_CYAN, font=get_font(16, bold=True))

    # Countdown and Action Button
    draw.rectangle([660, 700, 1260, 704], fill=(60, 30, 40))
    draw.text((820, 725), "Respawn available in: 3s...", fill=ACCENT_GOLD, font=get_font(18, bold=True))
    
    # Large Respawn Button
    draw_button(draw, (760, 770, 1160, 835), "RESPAWN NOW [E]", is_primary=True, f_size=18)
    draw.text((860, 845), "Authority verified on server", fill=TEXT_MUTED, font=get_font(12))

    img.save(os.path.join(OUTPUT_DIR, "layout_death_screen.png"))
    print("Generated layout_death_screen.png")

# ==========================================================
# 3. PAUSE / ESCAPE MENU LAYOUT (1920 x 1080)
# ==========================================================
def generate_pause_menu_layout():
    img = Image.new("RGB", (1920, 1080), BG_DARK)
    draw = ImageDraw.Draw(img)

    # Simulated blurred game background behind overlay
    draw.rectangle([0, 0, 1920, 1080], fill=(10, 14, 20))
    for i in range(20):
        draw.line([(0, i*54), (1920, i*54)], fill=(15, 20, 30), width=1)

    draw_header(draw, "PROPOSED LAYOUT: IN-GAME SYSTEM MENU (WBP_PauseMenu)", "Layer: Menu | Input: ECommonInputMode::Menu (World Simulation Runs Concurrently)")

    # Centered Vertical Acrylic Drawer
    drawer = (710, 180, 1210, 920)
    draw_rounded_panel(draw, drawer, fill=(18, 24, 32), outline=PANEL_BORDER, radius=12, width=2)

    # Header
    draw.text((870, 215), "SYSTEM MENU", fill=TEXT_WHITE, font=get_font(24, bold=True))
    draw.text((850, 250), "Game Session: Online (DS)", fill=ACCENT_CYAN, font=get_font(14))
    draw.line([(760, 280), (1160, 280)], fill=PANEL_BORDER, width=1)

    # Status summary
    draw_rounded_panel(draw, (750, 300, 1170, 365), fill=(25, 32, 42), outline=(50, 60, 75))
    draw.text((770, 312), "Dedicated Server: EU-Central-1", fill=TEXT_MUTED, font=get_font(12))
    draw.text((770, 332), "Latency: 28ms  |  Packet Loss: 0.0%", fill=ACCENT_GREEN, font=get_font(12, bold=True))

    # Menu Buttons Stack
    buttons = [
        ("RESUME GAME", True),
        ("SETTINGS", False),
        ("CHARACTER SELECT", False),
        ("QUIT TO DESKTOP", False)
    ]
    for i, (btext, is_p) in enumerate(buttons):
        by = 395 + i * 85
        draw_button(draw, (750, by, 1170, by + 65), btext, is_primary=is_p, f_size=16)

    # Footer notice
    draw.line([(760, 770), (1160, 770)], fill=PANEL_BORDER, width=1)
    draw.text((770, 790), "NOTE: Opening this menu does not pause the world.", fill=ACCENT_AMBER, font=get_font(12, bold=True))
    draw.text((770, 810), "Enemies and hazards can still attack your character.", fill=TEXT_MUTED, font=get_font(12))

    draw.text((890, 860), "[ESC] Close Menu", fill=TEXT_MUTED, font=get_font(13))

    img.save(os.path.join(OUTPUT_DIR, "layout_pause_menu.png"))
    print("Generated layout_pause_menu.png")

# ==========================================================
# 4. SETTINGS SCREEN LAYOUT (1920 x 1080)
# ==========================================================
def generate_settings_screen_layout():
    img = Image.new("RGB", (1920, 1080), BG_DARK)
    draw = ImageDraw.Draw(img)

    draw_header(draw, "PROPOSED LAYOUT: AUDIO & VIDEO SETTINGS (WBP_SettingsMenu)", "Layer: Menu | Subsystems: UMHAudioSubsystem (RTPCs) & UGameUserSettings")

    # Main Settings Card
    card = (360, 140, 1560, 960)
    draw_rounded_panel(draw, card, fill=PANEL_BG, outline=PANEL_BORDER, radius=12, width=2)

    # Title & Tabs
    draw.text((400, 170), "SETTINGS & PREFERENCES", fill=TEXT_WHITE, font=get_font(24, bold=True))
    
    # Left Column: Audio (MHAudio RTPC Sliders)
    col1 = (400, 220, 940, 840)
    draw_rounded_panel(draw, col1, fill=(18, 22, 28), outline=(40, 48, 60))
    draw.text((430, 240), "AUDIO CONFIGURATION (MHAudio Subsystem)", fill=ACCENT_GOLD, font=get_font(16, bold=True))
    draw.line([(430, 270), (910, 270)], fill=PANEL_BORDER)

    sliders = [
        ("Master Volume", "MasterVolume", 0.90),
        ("Music Volume", "MusicVolume", 0.75),
        ("Sound Effects (SFX)", "SFXVolume", 0.85),
        ("Voice / Dialogue", "VoiceVolume", 1.00)
    ]
    for i, (label, rtpc, val) in enumerate(sliders):
        sy = 300 + i * 110
        draw.text((430, sy), label, fill=TEXT_WHITE, font=get_font(14, bold=True))
        draw.text((780, sy), f"{int(val*100)}%", fill=ACCENT_CYAN, font=get_font(14, bold=True))
        draw.text((430, sy + 22), f"RTPC: '{rtpc}'", fill=TEXT_MUTED, font=get_font(11))
        
        # Track & Thumb
        draw.rectangle([430, sy + 48, 910, sy + 58], fill=(35, 42, 52), outline=BUTTON_BORDER)
        draw.rectangle([430, sy + 48, 430 + int(480 * val), sy + 58], fill=ACCENT_CYAN)
        thumb_x = 430 + int(480 * val)
        draw.ellipse([thumb_x - 10, sy + 43, thumb_x + 10, sy + 63], fill=TEXT_WHITE, outline=ACCENT_CYAN, width=2)

    # Right Column: Display & Graphic Settings
    col2 = (980, 220, 1520, 840)
    draw_rounded_panel(draw, col2, fill=(18, 22, 28), outline=(40, 48, 60))
    draw.text((1010, 240), "DISPLAY & GRAPHICS (UGameUserSettings)", fill=ACCENT_GOLD, font=get_font(16, bold=True))
    draw.line([(1010, 270), (1490, 270)], fill=PANEL_BORDER)

    # Window Mode Toggle
    draw.text((1010, 300), "Display Window Mode:", fill=TEXT_WHITE, font=get_font(14, bold=True))
    draw.text((1010, 322), "Fullscreen vs. Windowed Borderless", fill=TEXT_MUTED, font=get_font(12))
    draw_button(draw, (1010, 350, 1490, 410), "MODE: FULLSCREEN (CLICK TO TOGGLE)", is_primary=False, f_size=14)

    # Resolution readouts
    draw.text((1010, 440), "Render Resolution:", fill=TEXT_WHITE, font=get_font(14, bold=True))
    draw_rounded_panel(draw, (1010, 470, 1490, 520), fill=(25, 30, 40), outline=BUTTON_BORDER)
    draw.text((1030, 485), "1920 x 1080 (16:9 Native)", fill=ACCENT_CYAN, font=get_font(14, bold=True))

    # Graphics quality
    draw.text((1010, 550), "Overall Quality Preset:", fill=TEXT_WHITE, font=get_font(14, bold=True))
    draw_rounded_panel(draw, (1010, 580, 1490, 630), fill=(25, 30, 40), outline=BUTTON_BORDER)
    draw.text((1030, 595), "Epic / High (Auto-Detected)", fill=TEXT_WHITE, font=get_font(14))

    # Bottom Actions Bar
    draw.line([(400, 870), (1520, 870)], fill=PANEL_BORDER)
    draw_button(draw, (400, 890, 600, 940), "RESET DEFAULTS", is_primary=False, f_size=13)
    draw_button(draw, (1130, 890, 1310, 940), "BACK [ESC]", is_primary=False, f_size=14)
    draw_button(draw, (1330, 890, 1520, 940), "APPLY CHANGES", is_primary=True, f_size=14)

    img.save(os.path.join(OUTPUT_DIR, "layout_settings_screen.png"))
    print("Generated layout_settings_screen.png")

# ==========================================================
# 5. INVENTORY SCREEN LAYOUT (1920 x 1080)
# ==========================================================
def generate_inventory_screen_layout():
    img = Image.new("RGB", (1920, 1080), BG_DARK)
    draw = ImageDraw.Draw(img)

    draw_header(draw, "PROPOSED LAYOUT: INVENTORY & LOADOUT (WBP_InventoryScreen)", "Layer: Game | Equipment Paperdoll + Durability Indicators + 80-Slot Bag Grid")

    # Left Pane: Equipment Loadout & Character Paperdoll
    p_left = (120, 130, 780, 990)
    draw_rounded_panel(draw, p_left, fill=PANEL_BG, outline=PANEL_BORDER, radius=10)
    draw.text((150, 155), "EQUIPPED LOADOUT & STATS", fill=ACCENT_GOLD, font=get_font(18, bold=True))
    draw.text((150, 185), "Active Gear  |  Paperdoll Silhouette", fill=TEXT_MUTED, font=get_font(12))

    # Paperdoll slots (arranged)
    slots_left = [
        ("HEAD", 400, 230),
        ("NECK", 400, 310),
        ("CHEST", 400, 390),
        ("HANDS", 230, 390),
        ("WEAPON", 230, 480),
        ("LEGS", 400, 480),
        ("SHIELD", 570, 480),
        ("RING 1", 230, 570),
        ("FEET", 400, 570),
        ("RING 2", 570, 570),
        ("TRINKET", 400, 660)
    ]
    for sname, sx, sy in slots_left:
        draw_rounded_panel(draw, (sx, sy, sx + 90, sy + 70), fill=(25, 30, 40), outline=ACCENT_GOLD if "WEAPON" in sname else BUTTON_BORDER, radius=6)
        draw.text((sx + 10, sy + 15), sname, fill=TEXT_WHITE, font=get_font(11, bold=True))
        # Durability healthbar
        draw.rectangle([sx + 8, sy + 52, sx + 82, sy + 58], fill=(40, 20, 20))
        d_val = 0.85 if "WEAPON" in sname else 0.40 if "CHEST" in sname else 1.0
        draw.rectangle([sx + 8, sy + 52, sx + 8 + int(74 * d_val), sy + 58], fill=ACCENT_GREEN if d_val > 0.5 else ACCENT_AMBER)

    # Character Stats Box
    p_stats = (150, 755, 750, 885)
    draw_rounded_panel(draw, p_stats, fill=(18, 22, 28), outline=PANEL_BORDER)
    draw.text((170, 770), "COMBAT RATINGS:", fill=TEXT_MUTED, font=get_font(12, bold=True))
    draw.text((170, 795), "Attack Power: 1,420", fill=TEXT_WHITE, font=get_font(13))
    draw.text((170, 820), "Physical Armor: 840", fill=TEXT_WHITE, font=get_font(13))
    draw.text((450, 795), "Elemental Resist: 32%", fill=TEXT_WHITE, font=get_font(13))
    draw.text((450, 820), "Critical Rate: 24.5%", fill=TEXT_WHITE, font=get_font(13))

    # Repair All Button
    draw_button(draw, (150, 905, 750, 965), "REPAIR ALL EQUIPPED (Cost: 140 Scrap Metal)", is_primary=True, f_size=15)

    # Right Pane: Bag Grid (80 Slots)
    p_right = (820, 130, 1800, 990)
    draw_rounded_panel(draw, p_right, fill=PANEL_BG, outline=PANEL_BORDER, radius=10)
    draw.text((850, 155), "INVENTORY SATCHEL (80 SLOTS)", fill=ACCENT_GOLD, font=get_font(18, bold=True))
    draw.text((1580, 155), "Scrap Metal: 1,850", fill=ACCENT_CYAN, font=get_font(16, bold=True))

    # Filter tabs
    tabs = ["ALL (42/80)", "EQUIPMENT", "CONSUMABLE", "MATERIAL", "QUEST"]
    for i, t in enumerate(tabs):
        tx = 850 + i * 140
        draw_rounded_panel(draw, (tx, 195, tx + 130, 230), fill=BUTTON_BG if i == 0 else PANEL_BG, outline=ACCENT_GOLD if i == 0 else PANEL_BORDER, radius=4)
        draw.text((tx + 12, 203), t, fill=TEXT_WHITE if i == 0 else TEXT_MUTED, font=get_font(11, bold=True))

    # UniformGrid 8 columns x 8 visible rows
    grid_start_x = 850
    grid_start_y = 250
    cell_size = 90
    gap = 12

    for row in range(7):
        for col in range(9):
            cx = grid_start_x + col * (cell_size + gap)
            cy = grid_start_y + row * (cell_size + gap)
            
            # Color grade rings
            border_col = BUTTON_BORDER
            fill_col = (20, 25, 33)
            if row == 0 and col == 0:
                border_col = ACCENT_PURPLE  # Grade S
                fill_col = (30, 20, 40)
            elif row == 0 and col == 1:
                border_col = ACCENT_CYAN    # Grade A
                fill_col = (20, 30, 40)
            elif row == 0 and col == 2:
                border_col = ACCENT_GOLD    # Grade B
                fill_col = (35, 30, 20)

            draw_rounded_panel(draw, (cx, cy, cx + cell_size, cy + cell_size), fill=fill_col, outline=border_col, radius=6)
            
            # Simulated item badge
            if row == 0 and col == 0:
                draw.text((cx + 10, cy + 12), "Voidblade", fill=TEXT_WHITE, font=get_font(11, bold=True))
                draw.text((cx + 10, cy + 30), "Grade S", fill=ACCENT_PURPLE, font=get_font(10, bold=True))
                # Durability mini bar
                draw.rectangle([cx + 10, cy + 72, cx + 80, cy + 76], fill=(40, 20, 20))
                draw.rectangle([cx + 10, cy + 72, cx + 70, cy + 76], fill=ACCENT_GREEN)
            elif row == 0 and col == 1:
                draw.text((cx + 10, cy + 12), "Chestplate", fill=TEXT_WHITE, font=get_font(11, bold=True))
                draw.text((cx + 10, cy + 30), "Grade A", fill=ACCENT_CYAN, font=get_font(10, bold=True))
                draw.rectangle([cx + 10, cy + 72, cx + 80, cy + 76], fill=(40, 20, 20))
                draw.rectangle([cx + 10, cy + 72, cx + 40, cy + 76], fill=ACCENT_AMBER)
            elif row < 2 and col < 6:
                draw.text((cx + 25, cy + 30), "Item", fill=(60, 70, 85), font=get_font(12))

    draw.text((850, 960), "Tip: Left-Click item to inspect and open the Action Menu (Equip, Salvage, Infuse, Repair).", fill=TEXT_MUTED, font=get_font(12))

    img.save(os.path.join(OUTPUT_DIR, "layout_inventory_screen.png"))
    print("Generated layout_inventory_screen.png")

# ==========================================================
# 6. TOOLTIP & ACTION MENU CLOSE-UP (1400 x 900)
# ==========================================================
def generate_tooltip_action_layout():
    img = Image.new("RGB", (1400, 900), BG_DARK)
    draw = ImageDraw.Draw(img)

    draw_header(draw, "PROPOSED LAYOUT: ITEM TOOLTIP & CONTEXT ACTION MENU", "Widgets: WBP_ItemTooltip & WBP_ItemActionMenu | Durability + Infusion + Salvage Actions", width=1400)

    # Left: Tooltip Card (WBP_ItemTooltip)
    card_tt = (100, 130, 680, 850)
    draw_rounded_panel(draw, card_tt, fill=(18, 22, 28), outline=ACCENT_PURPLE, radius=10, width=2)

    # Tooltip Header
    draw.rectangle([102, 132, 678, 200], fill=(32, 20, 42))
    draw.text((130, 145), "Astral Voidblade +2", fill=TEXT_WHITE, font=get_font(22, bold=True))
    draw.text((130, 175), "Two-Handed Greatsword  |  Equipment", fill=TEXT_MUTED, font=get_font(12))

    # Grade Badge
    draw_rounded_panel(draw, (530, 145, 650, 185), fill=(50, 20, 70), outline=ACCENT_PURPLE, radius=6)
    draw.text((545, 153), "GRADE S", fill=ACCENT_PURPLE, font=get_font(13, bold=True))

    # Durability Bar
    draw.text((130, 220), "EQUIPMENT DURABILITY:", fill=TEXT_MUTED, font=get_font(12, bold=True))
    draw.text((540, 220), "85 / 100 (85%)", fill=ACCENT_GREEN, font=get_font(12, bold=True))
    draw.rectangle([130, 245, 650, 260], fill=(40, 20, 20), outline=BUTTON_BORDER)
    draw.rectangle([130, 245, 572, 260], fill=ACCENT_GREEN)

    # Malfunctioning Alert (Inactive preview)
    draw_rounded_panel(draw, (130, 275, 650, 315), fill=(30, 20, 20), outline=(80, 30, 30))
    draw.text((150, 285), "STATUS: NORMAL (Malfunction at 0% = -50% Effectiveness)", fill=(120, 80, 80), font=get_font(11))

    # Primary Combat Stats
    draw.text((130, 340), "PRIMARY COMBAT RATING:", fill=TEXT_MUTED, font=get_font(12, bold=True))
    draw_rounded_panel(draw, (130, 365, 650, 430), fill=(25, 30, 40), outline=PANEL_BORDER)
    draw.text((150, 380), "Weapon Physical Attack: +182", fill=ACCENT_GOLD, font=get_font(18, bold=True))
    draw.text((150, 405), "Attack Speed: 1.25 swings/sec", fill=TEXT_MUTED, font=get_font(12))

    # Secondary Rolled Affixes
    draw.text((130, 455), "ROLLED SECONDARY AFFIXES:", fill=TEXT_MUTED, font=get_font(12, bold=True))
    affixes = [
        ("+14.2% Critical Strike Damage", ACCENT_CYAN),
        ("+8.5% Void Element Attack", ACCENT_PURPLE),
        ("+120 Maximum Health", ACCENT_GREEN)
    ]
    for i, (aff, col) in enumerate(affixes):
        draw.text((150, 485 + i * 28), f"*  {aff}", fill=col, font=get_font(13, bold=True))

    # Infusion Level & Progress
    draw.text((130, 590), "DUPLICATE INFUSION RANK:", fill=TEXT_MUTED, font=get_font(12, bold=True))
    draw_rounded_panel(draw, (130, 615, 650, 700), fill=(25, 28, 38), outline=ACCENT_GOLD)
    draw.text((150, 630), "Current Infusion: Rank +2 / +5", fill=ACCENT_GOLD, font=get_font(14, bold=True))
    draw.text((150, 652), "Progress to Rank +3: 1 / 2 Duplicates Infused", fill=TEXT_MUTED, font=get_font(12))
    draw.rectangle([150, 675, 630, 685], fill=(35, 40, 50))
    draw.rectangle([150, 675, 390, 685], fill=ACCENT_GOLD)

    # Lore description
    draw.line([(130, 730), (650, 730)], fill=PANEL_BORDER)
    draw.text((130, 750), "\"Forged in the outer vortex before the fall of the academy.\"", fill=(100, 110, 125), font=get_font(12))
    draw.text((130, 780), "Bound to Player: Soulbound", fill=TEXT_MUTED, font=get_font(11))

    # Right: Context Action Menu (WBP_ItemActionMenu)
    card_act = (780, 220, 1280, 760)
    draw_rounded_panel(draw, card_act, fill=(22, 27, 36), outline=ACCENT_GOLD, radius=10, width=2)

    # Header
    draw.rectangle([782, 222, 1278, 290], fill=(35, 30, 20))
    draw.text((810, 238), "ITEM ACTIONS", fill=ACCENT_GOLD, font=get_font(16, bold=True))
    draw.text((810, 262), "Selected: Astral Voidblade +2", fill=TEXT_WHITE, font=get_font(13))

    actions = [
        ("EQUIP TO WEAPON SLOT", True),
        ("REPAIR ITEM (Cost: 25 Scrap)", False),
        ("INFUSE DUPLICATE (+1 Roll Tier)", False),
        ("SALVAGE FOR SCRAP (Yield: 85 Scrap)", False),
        ("CANCEL / CLOSE", False)
    ]
    for i, (atext, is_p) in enumerate(actions):
        ay = 315 + i * 80
        draw_button(draw, (810, ay, 1250, ay + 62), atext, is_primary=is_p, f_size=14)

    img.save(os.path.join(OUTPUT_DIR, "layout_item_tooltip_action.png"))
    print("Generated layout_item_tooltip_action.png")

if __name__ == "__main__":
    generate_hud_layout()
    generate_death_screen_layout()
    generate_pause_menu_layout()
    generate_settings_screen_layout()
    generate_inventory_screen_layout()
    generate_tooltip_action_layout()
    print("All layout mockups generated successfully!")
