#!/usr/bin/env python3
"""
Generate a DAW-themed icon for WavPlayer
Features: Waveform visualization with modern gradient colors
"""

from PIL import Image, ImageDraw
import math

def create_icon(size):
    """Create a single icon at the specified size"""
    # Create image with transparent background
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Define colors - modern DAW theme (dark blue to teal gradient feel)
    bg_color = (25, 35, 50, 255)  # Dark blue-gray background
    wave_color = (64, 156, 255, 255)  # Bright blue for waveform
    accent_color = (100, 200, 255, 255)  # Light blue accent

    # Draw rounded rectangle background
    margin = size // 8
    corner_radius = size // 6
    draw.rounded_rectangle(
        [(margin, margin), (size - margin, size - margin)],
        radius=corner_radius,
        fill=bg_color
    )

    # Draw waveform
    wave_margin = size // 4
    wave_width = size - 2 * wave_margin
    wave_height = size - 2 * wave_margin
    center_y = size // 2

    # Create waveform points
    num_points = max(20, size // 4)
    points = []

    for i in range(num_points):
        x = wave_margin + (i * wave_width / (num_points - 1))

        # Create interesting waveform pattern
        # Mix of sine waves at different frequencies
        t = i / num_points * 4 * math.pi
        amplitude = math.sin(t) * 0.6 + math.sin(t * 2.3) * 0.25 + math.sin(t * 0.7) * 0.15

        # Scale to wave height
        y = center_y + amplitude * (wave_height / 3)
        points.append((x, y))

    # Draw waveform line (thicker for larger icons)
    line_width = max(2, size // 32)

    # Draw the waveform
    for i in range(len(points) - 1):
        draw.line([points[i], points[i + 1]], fill=wave_color, width=line_width)

    # Add dots at peak points for visual interest
    if size >= 48:
        dot_size = max(2, size // 40)
        for i, (x, y) in enumerate(points):
            if i % 3 == 0:  # Only some points
                draw.ellipse(
                    [(x - dot_size, y - dot_size), (x + dot_size, y + dot_size)],
                    fill=accent_color
                )

    # Draw center line for reference
    center_line_width = max(1, size // 64)
    draw.line(
        [(wave_margin, center_y), (size - wave_margin, center_y)],
        fill=(80, 90, 110, 128),
        width=center_line_width
    )

    return img

def main():
    """Generate multi-resolution ICO file"""
    # Generate icons at standard Windows icon sizes (ICO format supports up to 256x256)
    ico_sizes = [16, 24, 32, 48, 64, 96, 128, 256]
    ico_images = [create_icon(size) for size in ico_sizes]

    # Save as ICO file with multiple resolutions
    # Note: Pillow has limitations with multi-size ICO files
    # We'll save the most common sizes in BMP format for better compatibility
    output_path = 'WavPlayer.ico'

    # Try saving with bitmap format for better Windows compatibility
    try:
        # Create a new list with converted images
        bmp_images = []
        for img in ico_images:
            # Convert RGBA to RGB with white background for BMP compatibility
            if img.mode == 'RGBA':
                rgb_img = Image.new('RGB', img.size, (255, 255, 255))
                rgb_img.paste(img, mask=img.split()[3])  # Use alpha channel as mask
                bmp_images.append(img)  # Keep RGBA for modern ICO format
            else:
                bmp_images.append(img)

        # Save with multiple sizes
        bmp_images[0].save(
            output_path,
            format='ICO',
            sizes=[(s, s) for s in ico_sizes]
        )
        print(f"✓ Generated {output_path} with {len(ico_sizes)} sizes: {ico_sizes}")
    except Exception as e:
        print(f"Warning: ICO multi-size save failed: {e}")
        # Fallback: save just the 48x48 version
        ico_images[3].save(output_path, format='ICO')
        print(f"✓ Generated {output_path} (fallback: 48x48 only)")

    # Also create a 512x512 version for high-DPI displays
    large_icon = create_icon(512)

    # Save preview PNGs at useful sizes
    large_icon.save('icon_preview.png')
    print(f"✓ Generated icon_preview.png (512x512) for preview")

    # Save a 256x256 PNG for About dialog and other uses
    ico_images[-1].save('icon_256.png')
    print(f"✓ Generated icon_256.png (256x256) for About dialog")

    # Save a 48x48 PNG for smaller displays
    ico_images[3].save('icon_48.png')
    print(f"✓ Generated icon_48.png (48x48) for small displays")

if __name__ == '__main__':
    main()
