import gdown
import os
import sys

# --- Configuration ---
# !!! REPLACE THIS WITH YOUR ACTUAL GOOGLE DRIVE SHARED LINK !!!
# Example formats:
# shared_link = "https://drive.google.com/file/d/YOUR_FILE_ID/view?usp=sharing"
# shared_link = "https://drive.google.com/uc?id=YOUR_FILE_ID"
# shared_link = "https://drive.google.com/open?id=YOUR_FILE_ID"

shared_link = (
    "https://drive.google.com/file/d/1ldC0kWpWzJ2fXQhDjyziwmFRIYeXwLBy/view?usp=sharing"
)

# --- Script Logic ---


def download_from_gdrive(url):
    """
    Downloads a file from a Google Drive shared link to the current directory,
    preserving the original filename.
    """
    print(f"Attempting to download from URL: {url}")

    # Basic check if the placeholder URL is still there
    if "YOUR_GOOGLE_DRIVE_SHARED_LINK_HERE" in url or "drive.google.com" not in url:
        print(
            "\nError: Please replace 'YOUR_GOOGLE_DRIVE_SHARED_LINK_HERE' "
            "with your actual Google Drive shared link in the script.",
            file=sys.stderr,
        )
        return None

    try:
        # gdown.download handles extracting the file ID and downloading.
        # Setting output=None (or omitting it) makes gdown save the file
        # to the current directory using the filename from Google Drive.
        # quiet=False shows download progress.
        # fuzzy=True helps parse various GDrive URL formats.
        print("Starting download (this might take a moment for large files)...")
        downloaded_path = gdown.download(url=url, output=None, quiet=False, fuzzy=True)

        if downloaded_path and os.path.exists(downloaded_path):
            print(
                f"\nSuccessfully downloaded and saved file as: '{os.path.basename(downloaded_path)}' "
                f"in directory: '{os.path.dirname(os.path.abspath(downloaded_path))}'"
            )
            return downloaded_path
        elif downloaded_path is None:
            # gdown returns None if download fails (e.g., permission error, invalid link)
            print("\nError: Download failed. gdown returned None.", file=sys.stderr)
            print(
                "Common reasons include incorrect URL, insufficient permissions "
                "(set sharing to 'Anyone with the link'), or network issues.",
                file=sys.stderr,
            )
            return None
        else:
            # This case should be rare if gdown returns a path but it doesn't exist
            print(
                f"\nWarning: gdown reported path '{downloaded_path}', but the file doesn't seem to exist.",
                file=sys.stderr,
            )
            return None

    except Exception as e:
        print(f"\nAn error occurred during the download process: {e}", file=sys.stderr)
        print("Please double-check:", file=sys.stderr)
        print("1. Your internet connection.", file=sys.stderr)
        print(
            "2. The Google Drive shared link is correct and accessible.",
            file=sys.stderr,
        )
        print(
            "3. File sharing permissions are set to 'Anyone with the link'.",
            file=sys.stderr,
        )
        print(
            "4. The 'gdown' library is installed (`pip install gdown`).",
            file=sys.stderr,
        )
        return None


# --- Main Execution ---
if __name__ == "__main__":
    downloaded_file = download_from_gdrive(shared_link)

    if downloaded_file:
        print("\nDownload process finished successfully.")
    else:
        print("\nDownload process finished with errors.")
