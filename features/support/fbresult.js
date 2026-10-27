// Loads the flatbuffers JavaScript bundle used to decode `format=flatbuffers`
// responses, generating it first from include/engine/api/flatbuffers/*.fbs.
//
// The bundle is not committed: a stale copy silently decodes the old field
// layout, so it is rebuilt whenever the schemas change. The script returns
// immediately when the bundle is already up to date.
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');

try {
  execFileSync('bash', [path.join(root, 'scripts/regenerate_fbresult_js.sh')], {
    cwd: root,
    // Keep stdout quiet so it does not interleave with cucumber's formatter,
    // but let errors (such as "no flatc found") through.
    stdio: ['ignore', 'ignore', 'inherit'],
  });
} catch {
  throw new Error(
    'Failed to generate features/generated/fbresult.js from the flatbuffers ' +
      'schemas (see the message above). Run `npm run regenerate-fbresult` for details.'
  );
}

const bundle = pathToFileURL(path.join(root, 'features/generated/fbresult.js')).href;
export const { osrm } = await import(bundle);
