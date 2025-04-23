# Road Network Download Scripts (London & Shinjuku)

## Overview

This repository contains two Python scripts designed to obtain simplified
driving road networks for routing analysis (e.g., using A*). You can choose to
download data for:

* **Shinjuku, Tokyo, Japan:** A smaller network, suitable for testing.
* **London, UK:** A large, complex network for more comprehensive analysis.

The scripts offer two methods for obtaining the data:

1. **`download_process_osm.py`**: Downloads the latest road network data
directly from OpenStreetMap (OSM) using the `osmnx` library for the chosen
location(s). It then simplifies the network's topology and saves it as a
GraphML file.

2. **`download_from_gdrive.py`**: Downloads pre-processed, simplified road
network GraphML file(s) directly from Google Drive shared links using the
`gdown` library. **This requires you to add the correct Google Drive link for
the Shinjuku data.** If you select "Both", it will attempt to download the
files in parallel.

**Recommendation:**

If suitable pre-processed network files are available on Google Drive (and you
have added the necessary links), **using `download_from_gdrive.py` is the
strongly preferred method.**

* **Efficiency:** It's significantly faster as it skips the time-consuming
download and processing steps.
* **Resource Conservation:** It avoids putting unnecessary load on the free,
volunteer-run OpenStreetMap servers.

Use the OSM download script (`download_process_osm.py`) only when you
specifically need:

* The absolute latest, real-time data from OSM.
* To apply custom processing or filtering during the download phase.
* If the Google Drive links are unavailable or not updated.

## Scripts Usage

### 1. Download from OpenStreetMap (`download_process_osm.py`)

* **Purpose:** Fetches the latest driving network directly from OSM for
**Shinjuku**, **London**, or **both**, simplifies its topology, and saves it
locally as GraphML file(s).
* **Usage:**
  * Navigate to the script's directory in your terminal.
  * Run the script using: `python download_process_osm.py`
  * The script will prompt you to choose:
    * `1`: Shinjuku, Tokyo (Test Data)
    * `2`: London, UK (Full Data)
    * `3`: Both Shinjuku and London
    * `4`: Exit
  * Enter your choice and press Enter.
* **Output:** Creates GraphML file(s) (e.g.,
`shinjuku_tokyo_drive_simplified.graphml`, `london_drive_simplified.graphml`)
in the same directory for the selected location(s).
* **Important Note:** This script queries the live OSM servers. Downloading
large areas like London can take **significant time** (minutes to potentially
much longer). Please use this script responsibly.

### 2. Download from Google Drive (`download_from_gdrive.py`) - Preferred Method

* **Purpose:** Downloads pre-existing, simplified network GraphML file(s) for
**Shinjuku**, **London**, or **both** from specified Google Drive shared links.
If "Both" is selected, downloads are attempted in parallel. This is recommended
for speed if the files and links meet your needs.
* **Configuration:**
  * **Crucial:** Open the `download_from_gdrive.py` file.
  * Locate the `drive_files` dictionary near the top.
* **Usage:**
  * Ensure you have configured the Shinjuku link (if you intend to download
  it).
  * Navigate to the script's directory in your terminal.
  * Run the script using: `python download_from_gdrive.py`
  * The script will prompt you to choose:
    * `1`: Shinjuku, Tokyo (Requires link to be added!)
    * `2`: London, UK
    * `3`: Both (Parallel Download - Shinjuku requires link)
    * `4`: Exit
  * Enter your choice and press Enter.
* **Output:** Downloads the selected file(s) (e.g.,
`shinjuku_tokyo_drive_simplified.graphml`, `london_drive_simplified.graphml`)
into the current directory. Download progress may be less detailed in parallel
mode.
* **Important Note:** Ensure Google Drive file sharing permissions are set to
"Anyone with the link". The download might pause at the start; be patient.

## Choosing Which Script to Use

* **Use `download_from_gdrive.py` (Recommended):**
  * If suitable GraphML files are available via shared links (and you've added
  the Shinjuku link).
  * For the fastest results (especially with parallel downloads for "Both").
  * To avoid hitting OSM servers unnecessarily.
* **Use `download_process_osm.py`:**
  * If you need the very latest data not yet available via link.
  * If the Google Drive links are missing or broken.
  * If you need to modify the download filters or simplification process.

## Notes

* Both scripts require an active internet connection.
* Ensure you have sufficient disk space for the downloaded GraphML file(s).
London's network file can be several hundred megabytes.
* The `gdown` and `osmnx` libraries (along with their dependencies like
`networkx`, `pandas`, `geopandas`) need to be installed (`pip install gdown
osmnx`).
