import { execFileSync } from 'node:child_process';
import { defineRouteMiddleware } from '@astrojs/starlight/route-data';
import { SECTION_SOURCES } from '../sections.mjs';

const CONTENT_DIR = 'src/content/docs/';

/** The repository-relative path an entry was copied from, if it was copied. */
function sourcePath(filePath: string | undefined): string | undefined {
	if (!filePath?.startsWith(CONTENT_DIR)) return undefined;
	const [section, ...rest] = filePath.slice(CONTENT_DIR.length).split('/');
	const prefix = (SECTION_SOURCES as Record<string, string>)[section];
	return prefix === undefined ? undefined : prefix + rest.join('/');
}

let repoRoot: string | undefined;

/** When `path` was last committed, or undefined if git knows nothing of it. */
function lastCommitDate(path: string): Date | undefined {
	try {
		repoRoot ??= execFileSync('git', ['rev-parse', '--show-toplevel'], { encoding: 'utf8' }).trim();
		const seconds = execFileSync('git', ['log', '-1', '--format=%ct', '--', path], {
			cwd: repoRoot,
			encoding: 'utf8',
		}).trim();
		return seconds ? new Date(Number(seconds) * 1000) : undefined;
	} catch {
		return undefined;
	}
}

export const onRequest = defineRouteMiddleware((context) => {
	const route = context.locals.starlightRoute;
	const { filePath } = route.entry;
	const source = sourcePath(filePath);
	if (source === undefined) return;

	// Starlight builds the link as `editLink.baseUrl` + the copy's path.
	if (route.editUrl) {
		route.editUrl = new URL(route.editUrl.href.replace(encodeURI(filePath), encodeURI(source)));
	}
	if (!route.lastUpdated && route.entry.data.lastUpdated !== false) {
		route.lastUpdated = lastCommitDate(source);
	}
});
