# Road Network Download Scripts (currently for London)

## Overview

This repository contains two Python scripts designed to obtain a simplified
driving road network for London, suitable for routing analysis (e.g., using
A*).

1. **`download_process_osm.py`**: Downloads the latest road network data
    directly from OpenStreetMap (OSM) using the `osmnx` library. It then
    simplifies the network's topology and saves it as a GraphML file.

2. **`download_from_gdrive.py`**: Downloads a pre-processed, simplified road
    network GraphML file directly from a Google Drive shared link using the
    `gdown` library.

**Recommendation:**

If a suitable pre-processed network file is available on Google Drive, **using
`download_from_gdrive.py` is the strongly preferred method.**

* **Efficiency:** It's significantly faster as it skips the time-consuming
download and processing steps.

* **Resource Conservation:** It avoids putting unnecessary load on the free,
volunteer-run OpenStreetMap servers.

Use the OSM download script (`download_process_osm.py`) only when you
specifically need:

* The absolute latest, real-time data from OSM.

* Data for a different region not available via a shared link.

* To apply custom processing or filtering during the download phase.

## Scripts Usage

### 1. Download from OpenStreetMap (`download_process_osm.py`)

* **Purpose:** Fetches the latest driving network for a specified location
directly from OSM, simplifies its topology for efficient routing, and saves it
locally as a GraphML file.

* **Configuration:**

  * Open the `download_process_osm.py` file in a text editor.
  
  * Locate the configuration section near the top.
  
  * Modify the `place_name` variable to change the target location if needed
    (default is London).

  * You can also change the desired `output_filename`.

    ```python
    # --- Configuration --- 
    place_name = "London, UK" # <-- CHANGE THIS for a different location 
    output_filename = "london_drive_simplified.graphml" # <-- CHANGE THIS for a different output name
    ```

* **Usage:**

  * Navigate to the script's directory in your terminal.
  
  * Run the script using: ```python download_process_osm.py```
  
* **Output:** Creates a GraphML file (e.g., `london_drive_simplified.graphml`)
in the same directory containing the simplified network.

* **Important Note:** This script queries the live OSM servers (Nominatim for
geocoding, Overpass API for data). Downloading large, complex areas like London
can take **significant time** (minutes to potentially much longer) depending on
server load, your internet speed, and your computer's processing power. Please
use this script responsibly.

### 2. Download from Google Drive (`download_from_gdrive.py`) - Preferred Method

* **Purpose:** Downloads a pre-existing, simplified network GraphML file from a
specified Google Drive shared link. This is the recommended method for speed
and resource conservation if the file meets your needs.

* **Configuration:**
  * Open the `download_from_gdrive.py` file in a text editor.
  * Locate the configuration section near the top. The download link should be
  populated with a valid URL.
  
* **Usage:**
  * Navigate to the script's directory in your terminal.
  * Run the script using: ```python download_from_gdrive.py```
  
* **Output:** Downloads the file specified by the Google Drive link into the
current directory. The script preserves the original filename from Google Drive
(e.g., `london_drive_simplified.graphml`). It will display download progress,
including the current download speed.

* **Important Note:** The download may not seem to progress at the start, be
patient.

## Choosing Which Script to Use

* **Use `download_from_gdrive.py` (Recommended):**

  * If a suitable GraphML file is already available via a shared link.

  * For the fastest results.

  * To avoid hitting OSM servers unnecessarily.

* **Use `download_process_osm.py`:**

  * If you need the very latest data not yet available via link.

  * If you need data for a different location.

  * If you need to modify the download filters or simplification process.

## Notes

* Both scripts require an active internet connection.

* Ensure you have sufficient disk space for the downloaded GraphML file
(London's network file can be several hundred megabytes).
