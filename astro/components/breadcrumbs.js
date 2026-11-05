/**
 * Liara Engine — what a page's trail says.
 */

import { API_GROUPS } from '../api/groups.mjs';
import { SECTIONS, SECTION_LABELS } from '../sections.mjs';

const API_GROUP_LABELS = new Map(API_GROUPS.map((group) => [group.dir, group.label]));

function firstLink(entries) {
    for (const entry of entries ?? []) {
        if (entry.type === 'link') return entry.href;
        const found = firstLink(entry.entries);
        if (found) return found;
    }
    return undefined;
}

function sectionHref(sidebar, label) {
    const group = (sidebar ?? []).find((entry) => entry.type === 'group' && entry.label === label);
    return group ? firstLink(group.entries) : undefined;
}

/**
 * The trail for one page.
 *
 * @param {object} options
 * @param {string} options.base      Astro's `BASE_URL`, with its trailing slash.
 * @param {string} options.entryId   The route's collection id, e.g. `api/classes/x`.
 * @param {string} options.module    Human-readable module name.
 * @param {Array}  [options.sidebar] `Astro.locals.starlightRoute.sidebar`.
 * @param {boolean} [options.isRoot] Whether this *is* the module's landing
 *                                   page, rather than a page below it.
 * @returns {Array<{label, href?, home?, current?}>}
 */
export function breadcrumbTrail({ base, entryId, module, sidebar, isRoot = false }) {
    const trail = [
        { label: module, href: isRoot ? undefined : base, current: isRoot, home: true },
    ];

    const [section, group] = String(entryId ?? '').split('/');
    const label = SECTION_LABELS[section];
    if (!label) return trail;

    trail.push({ label, href: sectionHref(sidebar, label) });

    if (section === SECTIONS.api && API_GROUP_LABELS.has(group)) {
        trail.push({ label: API_GROUP_LABELS.get(group) });
    }
    return trail;
}
