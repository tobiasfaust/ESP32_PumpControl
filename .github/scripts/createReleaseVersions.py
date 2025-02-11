from myUtils import *
import argparse

parser = argparse.ArgumentParser()

# erwartete Parameter
parser.add_argument('-m', '--ManifestDir', type=str, help='The Path where all manifest:_all.json are located', default='manifests')
parser.add_argument('-v', '--TargetDir', type=str, help='Verzeichnis, wo releases.json geschrieben werden soll', default='web-installer')

# Parsen der Argumente
args = parser.parse_args()



def build_releasejson(root: str) -> list:
    """
    Function to search and extract relevant information from manifestAll-*.json files.
    The function traverses all subdirectories in the specified root directory, looking for files that 
    match the pattern 'manifestAll-*.json'. For each matching file, it extracts the 'version', 'stage', 
    'build', and 'variant' information, as well as chip families. It appends paths from the 'builds' section of 'filesAll.json. 
    It also associates asset information with their corresponding assets from 'assets.json'. 
    The extracted information is then compiled into a list of dictionaries and returned.
    
    Parameters:
        root (str): The root directory to iterate over.
    Returns:
        list: A list of releases with extracted information.
    """
    results = []  # Liste, um die Ergebnisse zu speichern
    
    # Durchlaufe alle Unterverzeichnisse im angegebenen Verzeichnis
    for dirpath, dirnames, filenames in os.walk(root):
        # Prüfe, ob eine Datei existiert die mit 'manifestAll-*.json' aufhört
        for filename in filenames:
            manifest_path = os.path.join(dirpath, filename)
            if filename.startswith('manifestAll-') and filename.endswith('.json'):
                # Öffne und lade die JSON-Daten aus der Datei
                manifest_data = read_json_file(manifest_path)
                if manifest_data is not None:
                        
                    # Extrahiere 'version' und 'stage' falls vorhanden
                    version = manifest_data.get('version', None)
                    stage = manifest_data.get('stage', None)
                    build = manifest_data.get('build', None)
                    variant = manifest_data.get('variant', None)
                    #releasetag = manifest_data.get('releasetag', None) 
                        
                    chipFamilies = set()
                    for b in manifest_data.get('builds', []):
                        chipFamily = b.get('chipFamily')
                        if chipFamily:
                            chipFamilies.add(chipFamily)
                        
                    # Extrahiere den ersten 'path' aus 'parts' falls vorhanden, 
                    # extrahiere daraus den Pfad
                    try:
                        path = manifest_data.get('builds', [])[0].get('parts', [])[0].get('path')
                        #path = os.path.join(os.path.dirname(path), filename)
                    except:
                        path = None
                    
                    # extrahiere zugehörigen builds aus filesAll.json
                    builds = []
                    files = os.path.join(dirpath, 'filesAll.json')
                    if os.path.isfile(files):
                        with open(files, 'r') as file:
                            items = json.loads(file.read())
                            for item in items:
                                if item.get('variant') == variant:
                                    builds = item.get('builds', [])
                                    break
                            
                        # hole aus assets.json für jede datei die dazu passende asset_id
                        f_asset_obj = os.path.join(dirpath, 'assets.json')
                        if os.path.isfile(f_asset_obj):
                            with open(f_asset_obj, 'r') as f_assets:
                                assets = json.loads(f_assets.read())
                                for b in builds:
                                    for part in b.get('parts', []):
                                        part_path = part.get('path')
                                        part_filename = os.path.basename(part_path)
                                        for asset in assets.get('assets', []):
                                            if asset.get('name') == part_filename:
                                                part['asset_apiUrl'] = asset.get('apiUrl')
                                                break   
                    
                    # Falls sowohl 'version', 'build' und 'stage' vorhanden sind, füge sie zum Ergebnis hinzu
                    if version is not None and stage is not None:
                        results.append({
                            'manifest': os.path.join(os.path.dirname(path), filename),
                            #'files': os.path.join(os.path.dirname(path), 'filesAll.json'),
                            'version': version,
                            'stage': stage,
                            'variant': variant,
                            'build': int(build) if build else 0,
                            'chipFamilies': list(chipFamilies),
                            'builds': builds
                        })
    
    return results



if args.ManifestDir:
    # lade die manifestAll Dateien erneut und erstelle als Basis die versions
    # zu jeder version muss nun der dazu passende builds teil aus filesAll.json hinzugefügt werden
    # jede Datei aus parts muss mit der asset_id aus assets.json ergänzt werden

    extracted_data = build_releasejson(args.ManifestDir)

    # Wenn Daten extrahiert wurden, speichere sie in der JSON-Datei 'versions.json'
    if extracted_data:
        save_results_to_json(extracted_data, os.path.join(args.TargetDir, 'releases.json'))
    else:
        print("Keine relevanten 'manifest_all.json' Dateien gefunden.")
    