from pathlib import Path


# Hier die Datei fest eintragen
INPUT_FILE = Path(r"C:\Users\manue\Projekte\MultiCommander-SDK\SDKSamples\ImageDimensionsPlugin\Tools\IMG_20241017_225333.jpg")

# Wie viele Bytes vom Anfang sollen in die TXT geschrieben werden?
BYTES_TO_READ = 512


def format_hexdump(data: bytes, bytes_per_line: int = 16) -> str:
    lines = []

    for offset in range(0, len(data), bytes_per_line):
        chunk = data[offset : offset + bytes_per_line]

        hex_part = " ".join(f"{byte:02X}" for byte in chunk)
        ascii_part = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in chunk)

        hex_part = hex_part.ljust(bytes_per_line * 3 - 1)
        lines.append(f"{offset:08X}  {hex_part}  |{ascii_part}|")

    return "\n".join(lines)


def read_u32_le(data: bytes, offset: int) -> int | None:
    if offset + 4 > len(data):
        return None
    return int.from_bytes(data[offset : offset + 4], "little")


def read_fourcc(data: bytes, offset: int) -> str | None:
    if offset + 4 > len(data):
        return None
    raw = data[offset : offset + 4]
    try:
        return raw.decode("ascii", errors="replace")
    except Exception:
        return None


def analyze_webp_header(data: bytes) -> list[str]:
    lines: list[str] = []

    if len(data) < 12:
        lines.append("Datei ist zu kurz fuer RIFF/WEBP-Grundstruktur.")
        return lines

    riff = read_fourcc(data, 0)
    riff_size = read_u32_le(data, 4)
    webp = read_fourcc(data, 8)

    lines.append("Grundstruktur:")
    lines.append(f"  Offset 0x00000000: FourCC = {riff!r}")
    lines.append(f"  Offset 0x00000004: RIFF-Groesse (little endian) = {riff_size}")
    lines.append(f"  Offset 0x00000008: Format = {webp!r}")

    if riff != "RIFF":
        lines.append("  WARNUNG: Kein gueltiger RIFF-Header.")
        return lines

    if webp != "WEBP":
        lines.append("  WARNUNG: RIFF vorhanden, aber Format ist nicht WEBP.")
        return lines

    if len(data) >= 20:
        first_chunk = read_fourcc(data, 12)
        first_chunk_size = read_u32_le(data, 16)
        lines.append("")
        lines.append("Erster Chunk:")
        lines.append(f"  Offset 0x0000000C: Chunk FourCC = {first_chunk!r}")
        lines.append(f"  Offset 0x00000010: Chunk-Groesse = {first_chunk_size}")

        if first_chunk == "VP8X" and len(data) >= 30:
            flags = data[20]
            width_minus_one = int.from_bytes(data[24:27], "little")
            height_minus_one = int.from_bytes(data[27:30], "little")
            width = width_minus_one + 1
            height = height_minus_one + 1

            lines.append("")
            lines.append("VP8X-Analyse:")
            lines.append(f"  Flags = 0x{flags:02X}")
            lines.append(f"  Canvas Width - 1  = {width_minus_one}")
            lines.append(f"  Canvas Height - 1 = {height_minus_one}")
            lines.append(f"  Canvas Width      = {width}")
            lines.append(f"  Canvas Height     = {height}")

        elif first_chunk == "VP8L" and len(data) >= 25:
            b0 = data[21]
            b1 = data[22]
            b2 = data[23]
            b3 = data[24]

            width = 1 + (((b1 & 0x3F) << 8) | b0)
            height = 1 + (((b3 & 0x0F) << 10) | (b2 << 2) | ((b1 & 0xC0) >> 6))

            lines.append("")
            lines.append("VP8L-Analyse:")
            lines.append(f"  Width  = {width}")
            lines.append(f"  Height = {height}")

        elif first_chunk == "VP8 ":
            lines.append("")
            lines.append("VP8-Chunk erkannt.")
            lines.append("  Die Dimensionen stecken tiefer im VP8-Bitstream.")
            lines.append("  Fuer den Vergleich reicht oft schon die Chunk-Struktur.")
    else:
        lines.append("")
        lines.append("Datei zu kurz, um ersten Chunk sicher zu lesen.")

    return lines


def main() -> None:
    input_file = INPUT_FILE

    if not input_file.exists():
        raise FileNotFoundError(f"Datei nicht gefunden: {input_file}")

    output_file = input_file.with_suffix(input_file.suffix + ".txt")

    full_size = input_file.stat().st_size

    with input_file.open("rb") as file_handle:
        data = file_handle.read(BYTES_TO_READ)

    report_lines: list[str] = []
    report_lines.append(f"Datei: {input_file}")
    report_lines.append(f"Dateigroesse: {full_size} Bytes")
    report_lines.append(f"Gelesene Bytes vom Anfang: {len(data)}")
    report_lines.append("")

    report_lines.extend(analyze_webp_header(data))
    report_lines.append("")
    report_lines.append("Hexdump:")
    report_lines.append(format_hexdump(data))

    output_file.write_text("\n".join(report_lines), encoding="utf-8")

    print(f"Fertig: {output_file}")


if __name__ == "__main__":
    main()
