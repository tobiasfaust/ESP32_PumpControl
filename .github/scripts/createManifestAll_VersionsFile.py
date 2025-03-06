import argparse, os
from pathlib import Path
from myUtils import *

parser = argparse.ArgumentParser()

# erwartete Parameter
parser.add_argument('--FwDir', type=str, help='Root Verzeichnis für alle Firmwares', default='web-installer/firmware')
parser.add_argument('--VersDir', type=str, help='Verzeichnis wo eine bestimmte Firware liegt', default='web-installer/firmware/v2.5.0-208-DEV')

# Parsen der Argumente
args = parser.parse_args()

if Path(args.VersDir).is_dir() and Path(args.FwDir).is_dir():
    # Benennen die Verzeichnisse um, die noch ein zip (-> Artifacts) als Endung haben
    renameDirs(args.VersDir)

    # Suche nach 'manifest.json'-Dateien und extrahiere daraus die Daten für 
    # das 'manifestAll.json' aller ESP Architekturen im Hauptverzeichnis der Version
    process_manifestAll(args.VersDir)

    # Suche nach 'manifest.json'-Dateien und extrahiere daraus die Daten für
    # das 'filesAll.json' aller ESP Architekturen und Varianten im Hauptverzeichnis der Version
    process_filesAll(args.VersDir)

    # Suche nach 'manifestAll.json'-Dateien und extrahiere daraus die Daten für die 'versions.json'
    extracted_data = search_manifests_and_extract_version(args.FwDir, False)

    # Lösche die ältesten Versionen
    deleteVersions(args.FwDir, 6, extracted_data)

    # lade die Versionen erneut
    extracted_data = search_manifests_and_extract_version(args.FwDir, False)

    # Wenn Daten extrahiert wurden, speichere sie in der JSON-Datei 'versions.json'
    if extracted_data:
        save_results_to_json(extracted_data, os.path.join(args.FwDir, 'versions.json'))
    else:
        print("Keine relevanten 'manifest_all.json' Dateien gefunden.")
    
else:
    print(f"Der Pfad {args.VersDir} oder {args.FwDir} ist nicht verfügbar")