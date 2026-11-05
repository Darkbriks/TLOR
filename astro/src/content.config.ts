import { defineCollection } from 'astro:content';
import { docsLoader } from '@astrojs/starlight/loaders';
import { docsSchema } from '@astrojs/starlight/schema';
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
};
