import { defineCollection } from 'astro:content';
import { docsLoader, i18nLoader } from '@astrojs/starlight/loaders';
import { docsSchema, i18nSchema } from '@astrojs/starlight/schema';
import { composeLoaders, doxygenLoader } from '@liara/starlight-preset/api';

const sourceUrl = 'https://github.com/darkbriks/TLOR/blob/main';

export const collections = {
	docs: defineCollection({
		loader: composeLoaders(
			docsLoader(),
			doxygenLoader({
				base: import.meta.env.BASE_URL,
				sourceUrl,
				required: process.env.LIARA_DOCS_API === 'true',
			}),
		),
		schema: docsSchema(),
	}),
	// Overrides for Starlight's UI strings, one file per language. en.json is
	// empty, which keeps every default; it exists because Starlight reads this
	// collection on every page, and Astro warns about a collection with no
	// entries.
	i18n: defineCollection({
		loader: i18nLoader(),
		schema: i18nSchema(),
	}),
};
