FROM ubuntu:24.04

# noninteractive frontend for apt
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages, plus pandoc and TeX for PDF generation
RUN apt-get update && apt-get install -y \
        build-essential \
        cmake \
        git \
        ca-certificates \
        pandoc \
        texlive-latex-recommended \
        texlive-latex-extra \
        texlive-fonts-recommended \
        texlive-xetex \
        fonts-dejavu-core \
        && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy repository into the image so we can build the PDFs during image build
COPY . /workspace

# Generate PDFs from markdown files if present. Use xelatex via pandoc.
# Produce SHADOW_MAPPING.pdf and Integrazione.pdf in /workspace.
RUN if [ -f SHADOW_MAPPING.md ]; then \
            pandoc SHADOW_MAPPING.md -o SHADOW_MAPPING.pdf --pdf-engine=xelatex || pandoc SHADOW_MAPPING.md -o SHADOW_MAPPING.pdf; \
        else echo "SHADOW_MAPPING.md not found, skipping"; fi && \
        if [ -f Integrazione.md ]; then \
            pandoc Integrazione.md -o Integrazione.pdf --pdf-engine=xelatex || pandoc Integrazione.md -o Integrazione.pdf; \
        else echo "Integrazione.md not found, skipping"; fi

CMD ["/bin/bash"]