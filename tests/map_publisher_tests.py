from pathlib import Path
import tempfile,zipfile,subprocess,hashlib,sys
Path("r2-test").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir="r2-test") as temp:
    root=Path(temp);assert root.resolve().is_relative_to(Path('r2-test').resolve());archive=root/'pack.zip';manifest=root/'manifest.txt'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:z.writestr('content/@language/test.ff',b'test map bytes')
    manifest.write_text('# UNBOUND_RELEASE_MANIFEST_V1\n#HQ|51|1.0.0|example\nUNIVERSAL|FULL|-|1.0.0|example\n')
    args=[sys.executable,'tools/publish_map_catalog.py','--zip',str(archive),'--manifest',str(manifest),'--id','pack-1','--name','Test Pack','--description','Test description','--version','1.1.0','--url','https://downloads.unc-bo2.fun/test.zip']
    subprocess.run(args,check=True,capture_output=True)
    text=manifest.read_text();assert hashlib.sha256(archive.read_bytes()).hexdigest() in text
    assert str(archive.stat().st_size) in text
    assert '#HQ|51|' in text and 'UNIVERSAL|FULL|' in text
    subprocess.run(args+['--from-version','1.0.0'],check=True,capture_output=True)
    assert 'PATCH|1.0.0|1.1.0' in manifest.read_text()
    with zipfile.ZipFile(archive,'w') as z:z.writestr('update/code/game.rpl',b'not a map')
    assert subprocess.run(args,capture_output=True).returncode!=0
print('Publisher helper: real ZIP CRC/hash/size, patch record and forbidden code path checks passed.')

