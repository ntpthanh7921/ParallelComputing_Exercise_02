import gdown
import os
import sys
import threading

# --- Configuration ---
# Define download details for each location
drive_files = {
    "shinjuku": {
        "url": "https://drive.google.com/file/d/1-zfn9KgKmur7F6t14hNe_OoTiOng_hJp/view?usp=sharing",
        "output_filename": "shinjuku_tokyo_drive_simplified.graphml",  # Expected name
    },
    "london": {
        "url": "https://drive.google.com/file/d/1ldC0kWpWzJ2fXQhDjyziwmFRIYeXwLBy/view?usp=sharing",
        "output_filename": "london_drive_simplified.graphml",  # Expected name
    },
}


# --- Script Logic ---


def download_from_gdrive(url, expected_filename):
    """
    Downloads a file from a Google Drive shared link to the current directory.
    Uses the expected_filename if gdown can't determine it automatically.
    Returns True if successful, False otherwise.
    """
    thread_name = threading.current_thread().name
    print(f"[{thread_name}] Starting download for: {expected_filename}")
    print(f"[{thread_name}] From URL: {url}")

    # Basic check if the placeholder URL is still there
    if "YOUR_" in url or "drive.google.com" not in url:
        print(
            f"\n[{thread_name}] Error: Placeholder URL detected for {expected_filename}. "
            "Please add the actual Google Drive shared link in the script.",
            file=sys.stderr,
        )
        return False  # Indicate failure

    try:
        print(f"[{thread_name}] Attempting download via gdown...")
        downloaded_path = gdown.download(
            url=url, output=None, quiet=True, fuzzy=True
        )  # quiet=True for less cluttered parallel output

        if downloaded_path and os.path.exists(downloaded_path):
            actual_filename = os.path.basename(downloaded_path)
            # Simple success message
            print(f"\n[{thread_name}] Successfully downloaded: '{actual_filename}'")

            # Optional: Rename if needed and track success
            if actual_filename != expected_filename:
                print(
                    f"[{thread_name}] Warning: Downloaded name '{actual_filename}' != expected '{expected_filename}'."
                )
                # Add renaming logic here if desired

            return True  # Indicate success

        elif downloaded_path is None:
            print(
                f"\n[{thread_name}] Error: Download failed for {expected_filename} (gdown returned None).",
                file=sys.stderr,
            )
            print(
                f"[{thread_name}] Check URL, permissions ('Anyone with link'), and network.",
                file=sys.stderr,
            )
            return False  # Indicate failure
        else:
            print(
                f"\n[{thread_name}] Warning: gdown reported path '{downloaded_path}' for {expected_filename}, but file doesn't exist.",
                file=sys.stderr,
            )
            return False  # Indicate failure

    except Exception as e:
        print(
            f"\n[{thread_name}] An error occurred during download for {expected_filename}: {e}",
            file=sys.stderr,
        )
        print(
            f"[{thread_name}] Check internet, URL validity, permissions, and gdown installation.",
            file=sys.stderr,
        )
        return False  # Indicate failure


# --- Interactive Choice ---
selected_keys = []
while True:
    print("\nSelect which pre-processed network(s) to download from Google Drive:")
    print("1: Shinjuku, Tokyo (Test Data)")
    print("2: London, UK (Full Data)")
    print("3: Both Shinjuku and London (Parallel Download)")  # Updated text
    print("4: Exit")
    choice = input("Enter your choice (1, 2, 3, or 4): ").strip()

    if choice == "1":
        selected_keys = ["shinjuku"]
        break
    elif choice == "2":
        selected_keys = ["london"]
        break
    elif choice == "3":
        selected_keys = ["shinjuku", "london"]
        break
    elif choice == "4":
        print("Exiting script.")
        sys.exit()
    else:
        print("Invalid choice. Please enter 1, 2, 3, or 4.")

# --- Main Execution ---
if not selected_keys:
    print("No files selected. Exiting.")
else:
    print(f"\nSelected: {', '.join(selected_keys)}")

    threads = []
    skipped_placeholders = False
    final_success_count = 0

    if len(selected_keys) > 1:
        print("Starting parallel downloads...")
        for key in selected_keys:
            if key in drive_files:
                config = drive_files[key]
                # Check for placeholder URL *before* creating thread
                if "YOUR_" in config["url"]:
                    print(
                        f"\nSkipping {config['output_filename']}: Please replace the placeholder URL in the script."
                    )
                    skipped_placeholders = True
                    continue  # Skip this file

                # Create and start a thread for each download
                thread = threading.Thread(
                    target=download_from_gdrive,
                    args=(config["url"], config["output_filename"]),
                    name=f"Thread-{key}",  # Give threads meaningful names
                )
                threads.append(thread)
                thread.start()
            else:
                print(f"Warning: Configuration for '{key}' not found. Skipping.")

        # Wait for all started threads to complete
        print("\nWaiting for downloads to finish...")
        for thread in threads:
            thread.join()  # This blocks until the thread finishes
        # Note: We aren't explicitly checking return values here for simplicity,
        # rely on printed messages from the function. Could be enhanced.

    elif len(selected_keys) == 1:
        # Run sequentially if only one is selected
        key = selected_keys[0]
        print(f"Starting sequential download for {key}...")
        if key in drive_files:
            config = drive_files[key]
            if "YOUR_" in config["url"]:
                print(
                    f"\nSkipping {config['output_filename']}: Please replace the placeholder URL in the script."
                )
                skipped_placeholders = True
            else:
                # Directly call the function
                download_from_gdrive(config["url"], config["output_filename"])
        else:
            print(f"Warning: Configuration for '{key}' not found. Skipping.")

    print("\n----------------------------------------")
    print("Overall download process finished.")
    if skipped_placeholders:
        print("Note: One or more downloads were skipped due to placeholder URLs.")
