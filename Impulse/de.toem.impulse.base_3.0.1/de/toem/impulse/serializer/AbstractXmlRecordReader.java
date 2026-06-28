/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

public abstract class AbstractXmlRecordReader
extends AbstractSingleDomainRecordReader {
    public AbstractXmlRecordReader() {
    }

    public AbstractXmlRecordReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return 0;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        return -1;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            factory.setValidating(false);
            SAXParser saxParser = factory.newSAXParser();
            saxParser.parse(in, (DefaultHandler)new Handler());
        }
        catch (Exception e) {
            SystemLog.log(e);
        }
    }

    public abstract void startDocument();

    public abstract void endDocument();

    public abstract void startElement(String var1, String var2, String var3, Attributes var4);

    public abstract void endElement(String var1, String var2, String var3);

    public abstract void characters(char[] var1, int var2, int var3);

    class Handler
    extends DefaultHandler {
        Handler() {
        }

        @Override
        public void startDocument() throws SAXException {
            AbstractXmlRecordReader.this.startDocument();
            super.startDocument();
        }

        @Override
        public void endDocument() throws SAXException {
            AbstractXmlRecordReader.this.endDocument();
            super.endDocument();
        }

        @Override
        public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
            AbstractXmlRecordReader.this.startElement(uri, localName, qName, attributes);
            super.startElement(uri, localName, qName, attributes);
        }

        @Override
        public void endElement(String uri, String localName, String qName) throws SAXException {
            AbstractXmlRecordReader.this.endElement(uri, localName, qName);
            super.endElement(uri, localName, qName);
        }

        @Override
        public void characters(char[] ch, int start, int length) throws SAXException {
            AbstractXmlRecordReader.this.characters(ch, start, length);
            super.characters(ch, start, length);
        }

        @Override
        public InputSource resolveEntity(String publicId, String systemId) throws SAXException, IOException {
            return new InputSource(new StringReader(""));
        }
    }
}

