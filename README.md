# image-steganography-lsb
Implemented image steganography in C using LSB bit manipulation on BMP images, enabling secure encoding and decoding of text data with efficient file handling and low-level operations.

# Image Steganography using LSB Encoding & Decoding

## 📌 Overview
This project implements image steganography by embedding a secret text file inside a BMP image using Least Significant Bit (LSB) manipulation. The encoded image visually remains unchanged while securely carrying hidden data.

## 🚀 Features
- Encode secret text into BMP image
- Decode hidden text from encoded image
- Preserves original image quality
- Works with standard 24-bit BMP files
- Command-line based execution

## 🛠️ Technologies Used
- C Programming
- File Handling
- Bitwise Operations
- Pointers
- Makefile

## ⚙️ How It Works
- Reads BMP image and skips header
- Encodes:
  - Magic string
  - Size of secret file
  - Secret data (bit-by-bit into LSB)
- Decodes:
  - Extracts magic string
  - Reads size
  - Retrieves original text

## 🧪 Usage

### Compile
